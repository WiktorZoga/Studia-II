import torch
import torch.nn.functional as F
from transformers import AutoModelForCausalLM, AutoTokenizer

MODEL_NAME = "sdadas/polish-gpt2-large"
# MODEL_NAME = "flax-community/papuGaPT2"

device = (
    "mps"
    if torch.backends.mps.is_available()
    else ("cuda" if torch.cuda.is_available() else "cpu")
)

tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME)
model = AutoModelForCausalLM.from_pretrained(MODEL_NAME).to(device)
model.eval()


def get_sentence_loss(text: str) -> float:
    inputs = tokenizer(text, return_tensors="pt").to(device)
    with torch.no_grad():
        outputs = model(**inputs, labels=inputs["input_ids"])
        return outputs.loss.item()


def classify_by_loss(review: str) -> str:
    cand_pos = f"{review} Opinia jest pozytywna."
    cand_neg = f"{review} Opinia jest negatywna."

    loss_pos = get_sentence_loss(cand_pos)
    loss_neg = get_sentence_loss(cand_neg)

    return "pozytywna" if loss_pos < loss_neg else "negatywna"


def classify_by_next_token(review: str) -> str:
    prompt = f"Opinia: \"{review}\"\nOcena:"
    inputs = tokenizer(prompt, return_tensors="pt").to(device)

    id_pos = tokenizer.encode(" pozytywna")[0]
    id_neg = tokenizer.encode(" negatywna")[0]

    with torch.no_grad():
        outputs = model(**inputs)
        next_token_logits = outputs.logits[0, -1, :]
    #     probs = F.softmax(next_token_logits, dim=-1)

    #     prob_pos = probs[id_pos].item()
    #     prob_neg = probs[id_neg].item()

    # return "pozytywna" if prob_pos > prob_neg else "negatywna"

    return "pozytywna" if next_token_logits[id_pos] > next_token_logits[id_neg] else "negatywna"


def evaluate(dataset: list[tuple[str, str]], method_func, method_name: str):
    correct = 0
    total = len(dataset)

    print(f"\n--- Ewaluacja: {method_name} ---")
    for review, true_label in dataset:
        pred = method_func(review)
        is_ok = pred == true_label
        correct += int(is_ok)
        # print(f"Pred: {pred:<10} | True: {true_label:<10} | {review[:50]}...")

    accuracy = 100.0 * correct / total
    print(f"Skuteczność ({method_name}): {accuracy:.2f}% ({correct}/{total})")
    return accuracy


if __name__ == "__main__":

    dataset = []

    label_mapping = {"GOOD" : "pozytywna", "BAD": "negatywna"}

    with open("reviews_for_task3.txt", "r", encoding="utf-8") as file:
        for line in file:
            line = line.strip()

            if not line:
                continue

            label, review = line.split(maxsplit=1)

            dataset.append((review, label_mapping[label]))

    evaluate(dataset, classify_by_loss, "Metoda 1: Całkowity Loss zdania")
    evaluate(dataset, classify_by_next_token, "Metoda 2: Logity następnego tokenu")