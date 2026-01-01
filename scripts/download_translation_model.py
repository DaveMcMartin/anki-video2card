#!/usr/bin/env python3

import shutil
import sys
from pathlib import Path

try:
    from huggingface_hub import snapshot_download
except ImportError as e:
    print(f"Error: Missing required Python packages.")
    print(f"Please install them with: pip install huggingface-hub")
    sys.exit(1)

SCRIPT_DIR = Path(__file__).parent.resolve()
PROJECT_ROOT = SCRIPT_DIR.parent
ASSETS_DIR = PROJECT_ROOT / "assets"
MODEL_NAME = "entai2965/sugoi-v4-ja-en-ctranslate2"
MODEL_OUTPUT_DIR = ASSETS_DIR / "translation_model"


def download_model():
    print(f"Downloading model: {MODEL_NAME}")
    print(f"This may take a while...")

    try:
        print(f"Downloading from Hugging Face Hub...")

        snapshot_download(
            repo_id=MODEL_NAME,
            local_dir=str(MODEL_OUTPUT_DIR),
            local_dir_use_symlinks=False,
        )

        print(f"Model downloaded successfully to {MODEL_OUTPUT_DIR}")
        return True

    except Exception as e:
        print(f"Error downloading model: {e}")
        return False


def main():
    print("=" * 60)
    print("entai2965/sugoi-v4-ja-en-ctranslate2 Model Downloader")
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
        sys.exit(1)

    print()
    print("=" * 60)
    print("SUCCESS!")
    print("=" * 60)
    print(f"Translation model ready at: {MODEL_OUTPUT_DIR}")
    print()


if __name__ == "__main__":
    main()
