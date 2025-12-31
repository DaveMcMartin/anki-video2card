#!/usr/bin/env python3

import os
import shutil
import sys
from pathlib import Path

try:
    import ctranslate2
    from huggingface_hub import snapshot_download
    from transformers import AutoModelForSeq2SeqLM, AutoTokenizer
except ImportError as e:
    print(f"Error: Missing required Python packages.")
    print(
        f"Please install them with: pip install huggingface-hub transformers ctranslate2 torch"
    )
    sys.exit(1)

SCRIPT_DIR = Path(__file__).parent.resolve()
PROJECT_ROOT = SCRIPT_DIR.parent
ASSETS_DIR = PROJECT_ROOT / "assets"
MODEL_NAME = "Helsinki-NLP/opus-mt-ja-en"
MODEL_OUTPUT_DIR = ASSETS_DIR / "translation_model"
TEMP_DIR = ASSETS_DIR / "temp_model_download"


def download_model():
    print(f"Downloading model: {MODEL_NAME}")
    print(f"This may take a while (approx. 300MB)...")

    TEMP_DIR.mkdir(parents=True, exist_ok=True)

    try:
        print(f"Downloading from Hugging Face Hub...")
        model = AutoModelForSeq2SeqLM.from_pretrained(MODEL_NAME)
        tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME)

        model.save_pretrained(TEMP_DIR)
        tokenizer.save_pretrained(TEMP_DIR)

        print(f"Model downloaded successfully to {TEMP_DIR}")
        return True

    except Exception as e:
        print(f"Error downloading model: {e}")
        return False


def convert_to_ctranslate2():
    print(f"Converting model to CTranslate2 format...")

    try:
        converter = ctranslate2.converters.TransformersConverter(
            model_name_or_path=str(TEMP_DIR),
            activation_scales=None,
            copy_files=[
                "tokenizer_config.json",
                "vocab.json",
                "source.spm",
                "target.spm",
            ],
        )

        MODEL_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

        converter.convert(
            output_dir=str(MODEL_OUTPUT_DIR), quantization="int8", force=True
        )

        print(f"Model converted successfully to {MODEL_OUTPUT_DIR}")
        return True

    except Exception as e:
        print(f"Error converting model: {e}")
        return False


def cleanup():
    print("Cleaning up temporary files...")
    if TEMP_DIR.exists():
        shutil.rmtree(TEMP_DIR)
    print("Cleanup complete")


def main():
    print("=" * 60)
    print("Helsinki-NLP/opus-mt-ja-en Model Downloader")
    print("=" * 60)
    print()

    ASSETS_DIR.mkdir(parents=True, exist_ok=True)

    if MODEL_OUTPUT_DIR.exists():
        response = input(
            f"Model already exists at {MODEL_OUTPUT_DIR}. Overwrite? (y/N): "
        )
        if response.lower() != "y":
            print("Aborted.")
            sys.exit(0)
        shutil.rmtree(MODEL_OUTPUT_DIR)

    success = download_model()
    if not success:
        cleanup()
        sys.exit(1)

    success = convert_to_ctranslate2()
    if not success:
        cleanup()
        sys.exit(1)

    cleanup()

    print()
    print("=" * 60)
    print("SUCCESS!")
    print("=" * 60)
    print(f"Translation model ready at: {MODEL_OUTPUT_DIR}")
    print()


if __name__ == "__main__":
    main()
