"""Upload a qcow2 file to Alibaba Cloud OSS.

Usage (called by CI):
    python3 scripts/ci/upload_to_oss.py <qcow2_path> <target> <arch>

Environment variables (from GitHub Secrets):
    OSS_REGION, OSS_ACCESS_KEY_ID, OSS_ACCESS_KEY_SECRET,
    OSS_BUCKET_NAME, OSS_PUBLIC_URL, OSS_TENBOX_IMAGES_DIR
"""

import os
import sys
from pathlib import Path

import oss2


def get_image_id(target: str, arch: str) -> str:
    """Derive the OSS image directory name from target and arch.

    Examples:
        rootfs-copaw + x86_64  -> copaw
        rootfs-copaw + arm64   -> copaw-arm64
        rootfs-openclaw + x86_64 -> openclaw
        rootfs-openclaw + arm64  -> openclaw-arm64
    """
    name = target.removeprefix("rootfs-")
    if arch == "arm64":
        name += "-arm64"
    return name


def main():
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <qcow2_path> <target> <arch>")
        sys.exit(1)

    qcow2_path = sys.argv[1]
    target = sys.argv[2]
    arch = sys.argv[3]

    region = os.environ["OSS_REGION"]
    endpoint = f"https://{region}.aliyuncs.com"
    auth = oss2.Auth(os.environ["OSS_ACCESS_KEY_ID"], os.environ["OSS_ACCESS_KEY_SECRET"])
    bucket = oss2.Bucket(auth, endpoint, os.environ["OSS_BUCKET_NAME"])

    images_dir = os.environ.get("OSS_TENBOX_IMAGES_DIR", "tenbox/images").strip("/")
    image_id = get_image_id(target, arch)
    filename = Path(qcow2_path).name
    oss_key = f"{images_dir}/{image_id}/{filename}"

    file_size = os.path.getsize(qcow2_path)
    print(f"Uploading {qcow2_path} ({file_size / 1024 / 1024:.1f} MB)")
    print(f"  -> oss://{os.environ['OSS_BUCKET_NAME']}/{oss_key}")

    oss2.resumable_upload(
        bucket, oss_key, qcow2_path,
        multipart_threshold=10 * 1024 * 1024,
        part_size=2 * 1024 * 1024,
        num_threads=4,
    )

    public_url = os.environ.get("OSS_PUBLIC_URL", "").rstrip("/")
    download_url = f"{public_url}/{oss_key}"
    print(f"Upload complete: {download_url}")

    output_file = os.environ.get("GITHUB_OUTPUT")
    if output_file:
        with open(output_file, "a") as f:
            f.write(f"download_url={download_url}\n")


if __name__ == "__main__":
    main()
