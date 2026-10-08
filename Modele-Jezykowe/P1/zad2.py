from collections import Counter

import torch
from transformers import logging, pipeline
from transformers import AutoModelForCausalLM, AutoTokenizer

logging.set_verbosity_error()

MODEL_NAME = "flax-community/papuGaPT2"
device = (
    "mps"
    if torch.backends.mps.is_available()
    else ("cuda" if torch.cuda.is_available() else "cpu")
)

tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME)
model = AutoModelForCausalLM.from_pretrained(MODEL_NAME).to(device)
model.eval()

def muliset_permutation(counter: Counter):
    if not counter:
        yield ()
        return

    for word in list(counter.keys()):
        counter[word] -= 1
        if counter[word] == 0:
            del counter[word]

        for rest in muliset_permutation(counter):
            yield (word,) + rest

        counter[word] += 1

def format_sentence(words: tuple[str, ...]) -> str:
    sentence = " ".join(words).lower()
    return sentence.capitalize() + "."

def score_sentence(sentence: str) -> float:
    inputs = tokenizer(sentence, return_tensors="pt").to(device)
    with torch.no_grad():
        outputs = model(**inputs, labels=inputs["input_ids"])
        return outputs.loss.item()

def rank_permutations(words: list[str], top_k: int = 5):
    clean_words = [w.strip(".,!?").lower() for w in words]
    word_counts = Counter(clean_words)

    candidates = [
        format_sentence(perm) for perm in muliset_permutation(word_counts)
    ]

    print(f"Liczba wariantów: {len(candidates)}")

    scored = [(score_sentence(s), s) for s in candidates]

    return sorted(scored, key=lambda x: x[0])[:top_k]

if __name__ == "__main__":
    test_mulisets = [
        ["babuleńka", "miała", "dwa", "rogate", "koziołki"],
        ["wiewiórki", "w", "parku", "zaczepiają", "przechodniów"],
        ["dzień", "dobry", "która", "jest", "teraz", "godzina"]
    ]
    for test_words in test_mulisets:
        print('-' * 50)

        best = rank_permutations(test_words, top_k=5)

        for rank, (loss, s) in enumerate(best, start=1):
            print(f"{rank}. [loss: {loss:.4f}] {s}")

