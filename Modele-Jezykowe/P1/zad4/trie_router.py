import re

class TrieNode:
    def __init__(self):
        self.children = {}
        self.count = 0

class QuestionTrie:
    def __init__(self):
        self.root = TrieNode()

    @staticmethod
    def tokenize(text):
        return re.findall(r"\w+", text.lower())

    def insert(self, question):
        node = self.root
        node.count += 1

        for word in self.tokenize(question):
            if word not in node.children:
                node.children[word] = TrieNode()
            node = node.children[word]
            node.count += 1

    def list_prefixes(self, min_depth=1, max_depth=4, min_count=5):
        results = []

        def visit(node, words):
            depth = len(words)
            if min_depth <= depth <= max_depth and node.count >= min_count:
                results.append((" ".join(words), node.count))

            if depth < max_depth:
                for word, child in node.children.items():
                    visit(child, words + [word])

        visit(self.root, [])
        return sorted(results, key=lambda item: (-item[1], item[0]))

    def match_prefix(self, question, prefixes):
        words = self.tokenize(question)

        for prefix in sorted(prefixes, key=lambda text: len(text.split()), reverse=True):
            prefix_words = prefix.split()
            if words[: len(prefix_words)] == prefix_words:
                return prefix

        return None
