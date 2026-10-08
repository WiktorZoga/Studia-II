from collections import Counter

from trie_router import QuestionTrie
from zad4 import GROUPS, get_group, load_data

def main():
    data = load_data()

    trie = QuestionTrie()
    for question, _ in data:
        trie.insert(question)

    print(f"Liczba pytań: {len(data)}\n")

    print("Najczęstsze prefiksy:")
    prefixes = trie.list_prefixes(min_depth=2, max_depth=4, min_count=5)
    for prefix, count in prefixes[:50]:
        print(f"{prefix:<40} {count}")

    counts = Counter()
    for question, _ in data:
        group, _ = get_group(question, trie)
        counts[group] += 1

    print("\nWybrane grupy:")
    for group, group_prefixes in GROUPS.items():
        print(f"{group:<12} {counts[group]:>4}   {', '.join(group_prefixes)}")
    print(f"{'inne':<12} {counts['inne']:>4}")


if __name__ == "__main__":
    main()
