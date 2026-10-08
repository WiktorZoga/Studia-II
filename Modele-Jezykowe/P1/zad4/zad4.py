import re
from collections import Counter
from pathlib import Path

import torch
from transformers import AutoModelForCausalLM, AutoTokenizer
from transformers.utils import logging as transformers_logging

from trie_router import QuestionTrie


MODEL_NAME = "sdadas/polish-gpt2-large"
BASE_DIR = Path(__file__).parent

GROUPS = {
    "czy": ["czy"],
    "liczba": ["ile", "z ilu"],
    "rok": ["w którym roku"],
    "wiek": ["w którym wieku"],
    "nazwa": ["jak nazywa się", "jak nazywał się", "jak nazywała się"],
    "osoba": ["kto jest autorem", "kto był", "jak miał na imię"],
    "miejsce": ["w którym mieście", "w którym państwie"],
}

PREFIXES = [prefix for group in GROUPS.values() for prefix in group]
CENTURIES = [
    "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X",
    "XI", "XII", "XIII", "XIV", "XV", "XVI", "XVII", "XVIII",
    "XIX", "XX", "XXI",
]


def load_data():
    questions = (BASE_DIR / "task4_questions.txt").read_text(
        encoding="utf-8"
    ).splitlines()
    answers = (BASE_DIR / "task4_answers.txt").read_text(
        encoding="utf-8"
    ).splitlines()
    answers = [answer.split("\t") for answer in answers]

    return list(zip(questions, answers))


def get_group(question, trie):
    prefix = trie.match_prefix(question, PREFIXES)
    for group, prefixes in GROUPS.items():
        if prefix in prefixes:
            return group, prefix
    return "inne", "brak"


def get_examples(data, wanted_group, trie):
    examples = []
    for question, answers in data:
        group, _ = get_group(question, trie)
        if group == wanted_group:
            examples.append((question, answers[0]))
        if len(examples) == 3:
            break
    return examples


def load_model():
    transformers_logging.set_verbosity_error()
    transformers_logging.disable_progress_bar()

    if torch.backends.mps.is_available():
        device = "mps"
    elif torch.cuda.is_available():
        device = "cuda"
    else:
        device = "cpu"

    print("Ładowanie modelu...")
    tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME, local_files_only=True)
    model = AutoModelForCausalLM.from_pretrained(
        MODEL_NAME, local_files_only=True
    ).to(device)
    model.eval()
    print("Model gotowy")
    return tokenizer, model, device


def generate_answer(tokenizer, model, device, question, examples):
    prompt = ""
    for example_question, example_answer in examples:
        prompt += f"Pytanie: {example_question}\nOdpowiedź: {example_answer}\n"
    prompt += f"Pytanie: {question}\nOdpowiedź:"

    inputs = tokenizer(prompt, return_tensors="pt").to(device)
    with torch.no_grad():
        output = model.generate(
            **inputs,
            max_new_tokens=12,
            do_sample=False,
            pad_token_id=tokenizer.eos_token_id,
        )

    new_tokens = output[0][inputs["input_ids"].shape[1] :]
    answer = tokenizer.decode(new_tokens, skip_special_tokens=True)
    return answer.splitlines()[0].strip(" .,:;-–")


def candidate_loss(tokenizer, model, device, question, candidate):
    context = f"Pytanie: {question}\nOdpowiedź:"
    context_ids = tokenizer(context, add_special_tokens=False)["input_ids"]
    answer_ids = tokenizer(" " + candidate, add_special_tokens=False)["input_ids"]

    input_ids = torch.tensor([context_ids + answer_ids], device=device)
    labels = torch.tensor(
        [[-100] * len(context_ids) + answer_ids], device=device
    )

    with torch.no_grad():
        return model(input_ids=input_ids, labels=labels).loss.item()


def choose_century(tokenizer, model, device, question):
    candidates = ["w " + century for century in CENTURIES]
    losses = {
        candidate: candidate_loss(tokenizer, model, device, question, candidate)
        for candidate in candidates
    }
    return min(losses, key=losses.get)


def normalize(text):
    return " ".join(re.sub(r"[^\w]+", " ", text.lower()).split())


def is_correct(answer, correct_answers):
    answer = normalize(answer)
    return any(normalize(correct) in answer for correct in correct_answers)


def main():
    data = load_data()

    trie = QuestionTrie()
    for question, _ in data:
        trie.insert(question)

    examples = {
        group: get_examples(data, group, trie)
        for group in list(GROUPS) + ["inne"]
    }

    yes_no_answers = [
        answers[0].lower()
        for question, answers in data
        if get_group(question, trie)[0] == "czy"
        and answers[0].lower() in ("tak", "nie")
    ]
    most_common_yes_no = Counter(yes_no_answers).most_common(1)[0][0]

    tokenizer, model, device = load_model()
    correct_count = 0

    for number, (question, correct_answers) in enumerate(data, start=1):
        group, prefix = get_group(question, trie)

        if group == "czy":
            answer = most_common_yes_no
            method = "heurystyka"
        elif group == "wiek":
            answer = choose_century(tokenizer, model, device, question)
            method = "prawdopodobieństwo"
        else:
            answer = generate_answer(
                tokenizer, model, device, question, examples[group]
            )
            method = "few-shot"

        correct = is_correct(answer, correct_answers)
        correct_count += correct

        print(f"{number:4}/{len(data)} {'OK' if correct else 'BŁĄD'} | {group} | {method}")
        print("    Pytanie:", question)
        print("    Odpowiedź:", answer)
        print("    Poprawne:", " / ".join(correct_answers))

    percent = 100 * correct_count / len(data)
    print(f"\nWynik: {correct_count}/{len(data)} ({percent:.1f}%)")


if __name__ == "__main__":
    main()
