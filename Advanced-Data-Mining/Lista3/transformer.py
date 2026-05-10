import torch
import torch.nn as nn
import math

class InputEmbeddings(nn.Module):
    def __init__(self, dim: int, vocabulary_size: int):
        super().__init__()
        self.dim = dim
        self.vocabulary_size = vocabulary_size
        self.embedding = nn.Embedding(vocabulary_size, dim)
    
    def forward(self, x):
        return self.embedding(x) * math.sqrt(self.dim)

class PositionalEncoding(nn.Module):
    def __init__(self, dim: int, seq: int, dropout: float):
        super().__init__()
        self.dim = dim
        self.seq = seq # number of unique tokens
        self.dropout = nn.Dropout(dropout)

        pe = torch.zeros(seq, dim)

        position = torch.arange(0, seq, dtype=torch.float).unsqueeze(1) # (seq, 1)

        frequency = torch.exp(torch.arange(0, dim, 2).float() * (-math.log(10000.0) / dim)) # (dim / 2) frequencies form geometric sequence

        pe[:, 0::2] = torch.sin(position * frequency) # sin(position * freq_{2i})
        pe[:, 1::2] = torch.cos(position * frequency) # cos(position * freq_{2i})

        pe = pe.unsqueeze(0) # (1, seq, dim) adding batch dimension for broadcasting

        self.register_buffer('pe', pe)

    def forward(self, x):
        x = x + (self.pe[:, :x.shape[1], :]).requires_grad_(False) # (batch, seq, dim)
        return self.dropout(x)

class LayerNormalization(nn.Module): # from residual connection
    def __init__(self, features: int, eps: float = 10**(-6)) -> None:
        super().__init__()
        self.eps = eps
        self.alpha = nn.Parameter(torch.ones(features)) # alpha (multiplicative) is a learnable parameter
        self.bias = nn.Parameter(torch.zeros(features)) # bias (additive) is a learnable parameter
    
    def forward(self, x):
        # x: (batch, seq, features)
        # Keep dims for broadcasting
        mean = x.mean(dim = -1, keepdim = True) # (batch, seq, 1) mean in last dim
        std = x.std(dim = -1, keepdim = True) # (batch, seq, 1) std in last dim
        # eps only for numerical stablity
        return self.alpha * (x - mean) / (std + self.eps) + self.bias
    
class FeedForwardBlock(nn.Module):
    def __init__(self, dim: int, dim_fc: int, dropout: float) -> None:
        super().__init__()
        self.linear_1 = nn.Linear(dim, dim_fc) # w1 and b1
        self.dropout = nn.Dropout(dropout)
        self.linear_2 = nn.Linear(dim_fc, dim) # w2 and b2
    
    def forward(self, x):
        # (batch, seq, dim) --> (batch, seq, dim_fc) --> (batch, seq, dim)
        return self.linear_2(self.dropout(torch.relu(self.linear_1(x))))

class MultiHeadAttentionBlock(nn.Module):
    def __init__(self, dim: int, h: int, dropout: float) -> None:
        super().__init__()
        self.dim = dim # embedding vector size
        self.h = h # number of heads
        assert dim % h == 0, "embedding vector size is not divisible by number of heads"

        self.dim_h = dim // h # dim of vector for each head

        self.w_q = nn.Linear(dim, dim, bias=False) 
        self.w_k = nn.Linear(dim, dim, bias=False) 
        self.w_v = nn.Linear(dim, dim, bias=False) 
        self.w_o = nn.Linear(dim, dim, bias=False) 

        self.dropout = nn.Dropout(dropout)
    
    @staticmethod
    def attention(query, key, value, mask, dropout: nn.Dropout):
        dim_h = query.shape[-1]
        # (batch, h, seq, dim_h) --> (batch, h, seq, seq)
        attention_scores = (query @ key.transpose(-2, -1)) / math.sqrt(dim_h)

        if mask is not None:
            attention_scores.masked_fill(mask == 0, -1e9)  # -1e9 for mask == 0 (e^-1e9 ~ 0)
        
        attention_scores = attention_scores.softmax(dim=-1) # (batch, h, seq, seq) apply softmax

        if dropout is not None:
            attention_scores = dropout(attention_scores)

        # (batch, h, seq, seq) --> (batch, h, seq, dim_head)
        # return attention scores which can be used for visualization
        return (attention_scores @ value), attention_scores

    def forward(self, q, k, v, mask):
        query = self.w_q(q) # (batch, seq, dim) --> (batch, seq, dim)
        key = self.w_k(k)   # (batch, seq, dim) --> (batch, seq, dim)
        value = self.w_v(v) # (batch, seq, dim) --> (batch, seq, dim)

        # (batch, seq, dim) --> (batch, seq, h, dim_head) --> (batch, h, seq, dim_head)


        query = query.view(query.shape[0], query.shape[1], self.h, self.dim_h).transpose(1, 2)
        key = key.view(key.shape[0], key.shape[1], self.h, self.dim_h).transpose(1, 2)
        value = value.view(value.shape[0], value.shape[1], self.h, self.dim_h).transpose(1, 2)

        # self attention

        x, self.attention_scores = MultiHeadAttentionBlock.attention(query, key, value, mask, self.dropout)

        # Combine all the heads together
        # (batch, h, seq, dim_h) --> (batch, seq, h, dim_h) --> (batch, seq, dim)
        x = x.transpose(1, 2).contiguous().view(x.shape[0], -1, self.h * self.dim_h)


        # Multiply by Wo

        return self.w_o(x)
    
class ResidualConnection(nn.Module):
    def __init__(self, features: int, dropout: float) -> None:
        super().__init__()
        self.norm = LayerNormalization(features)
        self.dropout = nn.Dropout(dropout)
    
    def forward(self, x, sublayer): # sublayer is this what this other part of connections skips
        return x + self.dropout(sublayer(self.norm(x)))
    
class EncoderBlock(nn.Module):
    def __init__(self, features: int, self_attention_block: MultiHeadAttentionBlock, feed_forward_block: FeedForwardBlock, dropout: float) -> None:
        super().__init__()
        self.self_attention_block = self_attention_block
        self.feed_forward_block = feed_forward_block
        self.residual_connections = nn.ModuleList([
            ResidualConnection(features, dropout),
            ResidualConnection(features, dropout)
        ])

    def forward(self, x, source_mask):
        x = self.residual_connections[0](x, lambda x: self.self_attention_block(x, x, x, source_mask)) # binding
        x = self.residual_connections[1](x, self.feed_forward_block)
        return x

class Encoder(nn.Module):
    def __init__(self, features: int, layers: nn.ModuleList) -> None:
        super().__init__()
        self.layers = layers
        self.norm = LayerNormalization(features)
    
    def forward(self, x, mask):
        for layer in self.layers:
            x = layer(x, mask)
        return self.norm(x)
    
class DecoderBlock(nn.Module):
    def __init__(self, features: int, self_attention_block: MultiHeadAttentionBlock, cross_attention_block: MultiHeadAttentionBlock, feed_forward_block: FeedForwardBlock, dropout: float) -> None:
        super().__init__()
        self.self_attention_block = self_attention_block
        self.cross_attention_block = cross_attention_block
        self.feed_forward_block = feed_forward_block
        self.residual_connections = nn.ModuleList([
            ResidualConnection(features, dropout),
            ResidualConnection(features, dropout),
            ResidualConnection(features, dropout)
        ])

    def forward(self, x, encoder_output, source_mask, target_mask):
        x = self.residual_connections[0](x, lambda x: self.self_attention_block(x, x, x, target_mask))
        x = self.residual_connections[1](x, lambda x: self.cross_attention_block(x, encoder_output, encoder_output, source_mask))
        x = self.residual_connections[2](x, self.feed_forward_block)
        return x

class Decoder(nn.Module):
    def __init__(self, features: int, layers: nn.ModuleList) -> None:
        super().__init__()
        self.layers = layers
        self.norm = LayerNormalization(features)

    def forward(self, x, encoder_output, source_mask, target_mask):
        for layer in self.layers:
            x = layer(x, encoder_output, source_mask, target_mask)
        return self.norm(x)

class ProjectionLayer(nn.Module):
    def __init__(self, dim, vocabulary_size) -> None:
        super().__init__()
        self.proj = nn.Linear(dim, vocabulary_size)

    def forward(self, x) -> None:
        # (batch, seq, dim) --> (batch, seq, vocabulary_size)
        return self.proj(x)

class Transformer(nn.Module):
    def __init__(self, encoder: Encoder, decoder: Decoder, 
                 source_embed: InputEmbeddings, target_embed: InputEmbeddings, 
                 source_pos: PositionalEncoding, target_pos: PositionalEncoding, 
                 projection_layer: ProjectionLayer) -> None:
        super().__init__()
        self.encoder = encoder
        self.decoder = decoder
        self.source_embed = source_embed
        self.target_embed = target_embed
        self.source_pos = source_pos
        self.target_pos = target_pos
        self.projection_layer = projection_layer
    
    def encode(self, source, source_mask):
        # (batch, seq, dim)
        source = self.source_embed(source)
        source = self.source_pos(source)
        return self.encoder(source, source_mask)
    
    def decode(self, encoder_output: torch.Tensor, source_mask: torch.Tensor, target: torch.Tensor, target_mask: torch.Tensor):
        # (batch, seq, dim)
        target = self.target_embed(target)
        target = self.target_pos(target)
        return self.decoder(target, encoder_output, source_mask, target_mask)
    
    def project(self, x):
        # (batch, seq, vocabulary_size)
        return self.projection_layer(x)
    

def build_transformer(source_vocabulary_size: int, target_vocabulary_size: int, 
                      source_seq: int, target_seq: int, 
                      dim: int=512, N: int=6, h: int=8, 
                      dropout: float=0.1, dim_fc: int=2048) -> Transformer:
    # Create the embedding layers
    source_embed = InputEmbeddings(dim, source_vocabulary_size)
    target_embed = InputEmbeddings(dim, target_vocabulary_size)

    # Create the positional encoding layers
    source_pos = PositionalEncoding(dim, source_seq, dropout)
    target_pos = PositionalEncoding(dim, target_seq, dropout)
    
    # Create the encoder blocks
    encoder_blocks = []
    for _ in range(N):
        encoder_self_attention_block = MultiHeadAttentionBlock(dim, h, dropout)
        feed_forward_block = FeedForwardBlock(dim, dim_fc, dropout)
        encoder_block = EncoderBlock(dim, encoder_self_attention_block, feed_forward_block, dropout)
        encoder_blocks.append(encoder_block)

    # Create the decoder blocks
    decoder_blocks = []
    for _ in range(N):
        decoder_self_attention_block = MultiHeadAttentionBlock(dim, h, dropout)
        decoder_cross_attention_block = MultiHeadAttentionBlock(dim, h, dropout)
        feed_forward_block = FeedForwardBlock(dim, dim_fc, dropout)
        decoder_block = DecoderBlock(dim, decoder_self_attention_block, decoder_cross_attention_block, feed_forward_block, dropout)
        decoder_blocks.append(decoder_block)
    
    # Create the encoder and decoder
    encoder = Encoder(dim, nn.ModuleList(encoder_blocks))
    decoder = Decoder(dim, nn.ModuleList(decoder_blocks))
    
    # Create the projection layer
    projection_layer = ProjectionLayer(dim, target_vocabulary_size)
    
    # Create the transformer
    transformer = Transformer(encoder, decoder, source_embed, target_embed, source_pos, target_pos, projection_layer)
    
    # Initialize the parameters
    for p in transformer.parameters():
        if p.dim() > 1:
            nn.init.xavier_uniform_(p) # jakaś dziwna nazwa na pewien rozkład jednostajny
    
    return transformer