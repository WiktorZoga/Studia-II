from transformers import logging, pipeline

logging.set_verbosity_error()

generator = pipeline(
    "text-generation",
    model="flax-community/papuGaPT2",
    device="mps",
)

history = "Tomek i Ania prowadzą rozmowę na forum internetowym."

while True:

    try:
        user_text = input("Tomek: ").strip()
    except:
        print("Koniec rozmowy.")
        break

    if not user_text:
        print("Koniec rozmowy.")
        break

    prompt = (
        f"{history}\n"
        f"Tomek: {user_text}\n"
        "Ania:"
    )

    results = generator(
        prompt,
        max_new_tokens=32,
        num_return_sequences=5,
        do_sample=True,
        temperature=0.8,
        top_p=0.9,
        pad_token_id=generator.tokenizer.eos_token_id,
    )

    candidates = []

    for result in results:
        answer = result["generated_text"][len(prompt):].strip()

        for separator in ["\n", "Tomek:", "Ania:", " - ", " – "]:
            answer = answer.split(separator)[0]

        answer = answer.strip("-–>: \"")

        if answer:
            candidates.append(answer)

    longer_candidates = [
        answer for answer in candidates
        if len(answer.split()) >= 3
    ]

    TARGET_WORDS = 10

    def length_penalty(cand):
        return abs(len(cand.split()) - TARGET_WORDS)

    answer = min(candidates, key=length_penalty)

    print(f"Ania: {answer}")

    history += (
        f"\nTomek: {user_text}"
        f"\nAnia: {answer}"
    )

    print("*"*50 +f"\nHistoria:\n`{history}" + "\n" + "*"*50)
