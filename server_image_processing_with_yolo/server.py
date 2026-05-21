import socket
import struct
import os
import time
from datetime import datetime
from pathlib import Path
import json
import csv

import numpy as np
from ultralytics import YOLO

"""
ESP32 taradından kaç tane resim gönderilirse model gelen kadarını işleyerek sonucu dönüyor.
"""


HOST = "0.0.0.0"
PORT = 5001

SAVE_DIR = Path.home() / "Desktop" / "esp32_photos"
MODEL_PATH = Path.home() / "Desktop" / "best.pt"

CONF_THRESHOLD = 0.60  # bir resmin doğrulanması için minimum treshold
STRONG_CONF_THRESHOLD = (
    0.70  # bu treshold'dan daha yüksek bir tane resim olursa sonuç pozitif
)
MIN_POSITIVE_FRAMES = (
    10  # çekilen resimlerden sonucu belirlemek için min pozitif resim sayısı
)
IMG_SIZE = 640

SAVE_DIR.mkdir(parents=True, exist_ok=True)

print("Model yükleniyor...")
model = YOLO(str(MODEL_PATH))
print(f"Model yüklendi: {MODEL_PATH}")


def recv_exact(conn, n):
    data = b""
    while len(data) < n:
        chunk = conn.recv(n - len(data))
        if not chunk:
            raise ConnectionError("Bağlantı kesildi")
        data += chunk
    return data


def analyze_burst(image_paths):
    start_time = time.time()

    results = model.predict(
        source=[str(p) for p in image_paths],
        imgsz=IMG_SIZE,
        conf=CONF_THRESHOLD,
        iou=0.5,
        verbose=False,
    )

    frame_scores = []
    frame_details = []

    positive_frames = 0
    max_conf = 0.0
    best_frame = None
    total_detections = 0

    class_summary = {}

    for frame_index, (img_path, r) in enumerate(zip(image_paths, results), start=1):
        detections = []
        frame_max = 0.0

        if r.boxes is not None and len(r.boxes) > 0:
            boxes = r.boxes

            confs = boxes.conf.cpu().numpy()
            classes = boxes.cls.cpu().numpy().astype(int)

            frame_max = float(np.max(confs))
            total_detections += len(confs)

            for det_index, (cls_id, conf) in enumerate(zip(classes, confs), start=1):
                class_name = model.names.get(int(cls_id), str(cls_id))

                detection_info = {
                    "detection_index": det_index,
                    "class_id": int(cls_id),
                    "class_name": class_name,
                    "confidence": round(float(conf), 4),
                }

                detections.append(detection_info)

                if class_name not in class_summary:
                    class_summary[class_name] = {"count": 0, "max_confidence": 0.0}

                class_summary[class_name]["count"] += 1
                class_summary[class_name]["max_confidence"] = max(
                    class_summary[class_name]["max_confidence"], float(conf)
                )

        frame_scores.append(frame_max)

        is_positive = frame_max >= CONF_THRESHOLD

        if is_positive:
            positive_frames += 1

        if frame_max > max_conf:
            max_conf = frame_max
            best_frame = img_path

        frame_details.append(
            {
                "frame_index": frame_index,
                "filename": img_path.name,
                "path": str(img_path),
                "is_positive": is_positive,
                "max_confidence": round(frame_max, 4),
                "detection_count": len(detections),
                "detections": detections,
            }
        )

    total_frames = len(frame_scores)
    positive_ratio = positive_frames / max(1, total_frames)

    detected_by_frame_count = positive_frames >= MIN_POSITIVE_FRAMES
    detected_by_strong_conf = max_conf >= STRONG_CONF_THRESHOLD

    detected = positive_frames >= MIN_POSITIVE_FRAMES and max_conf >= CONF_THRESHOLD

    event_score = (0.75 * max_conf) + (0.25 * min(1.0, positive_ratio * 3.0))
    elapsed = time.time() - start_time

    if detected_by_frame_count and detected_by_strong_conf:
        decision_reason = (
            f"Tespit yapıldı çünkü pozitif kare sayısı yeterli "
            f"({positive_frames} >= {MIN_POSITIVE_FRAMES}) ve maksimum confidence "
            f"güçlü eşik değerini geçti ({round(max_conf, 4)} >= {STRONG_CONF_THRESHOLD})."
        )
    elif (not detected_by_frame_count) and detected_by_strong_conf:
        decision_reason = (
            f"Tespit yapılmadı çünkü pozitif kare sayısı yetersiz "
            f"({positive_frames} >= {MIN_POSITIVE_FRAMES})"
        )
    elif detected_by_frame_count and (not detected_by_strong_conf):
        decision_reason = (
            f"Tespit yapılmadı çünkü maksimum confidence "
            f"güçlü eşik değerinin altında kaldı ({round(max_conf, 4)} < {STRONG_CONF_THRESHOLD})."
        )
    else:
        decision_reason = (
            f"Tespit yapılmadı çünkü pozitif kare sayısı yetersiz "
            f"({positive_frames} < {MIN_POSITIVE_FRAMES}) ve maksimum confidence "
            f"güçlü eşik değerinin altında kaldı ({round(max_conf, 4)} < {STRONG_CONF_THRESHOLD})."
        )

    for cls_name in class_summary:
        class_summary[cls_name]["max_confidence"] = round(
            class_summary[cls_name]["max_confidence"], 4
        )

    return {
        "detected": detected,
        "decision": "ARANAN CİSİM VAR" if detected else "ARANAN CİSİM YOK",
        "decision_reason": decision_reason,
        "probability_percent": round(event_score * 100, 2),
        "positive_frames": positive_frames,
        "negative_frames": total_frames - positive_frames,
        "total_frames": total_frames,
        "positive_ratio": round(positive_ratio, 3),
        "max_confidence": round(max_conf, 4),
        "best_frame": str(best_frame) if best_frame else None,
        "total_detections": total_detections,
        "class_summary": class_summary,
        "thresholds": {
            "conf_threshold": CONF_THRESHOLD,
            "strong_conf_threshold": STRONG_CONF_THRESHOLD,
            "min_positive_frames": MIN_POSITIVE_FRAMES,
            "img_size": IMG_SIZE,
        },
        "elapsed_seconds": round(elapsed, 3),
        "frame_scores": [round(x, 4) for x in frame_scores],
        "frame_details": frame_details,
    }


server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind((HOST, PORT))
server.listen(5)

print(f"Bekleniyor... Fotoğraflar: {SAVE_DIR}")
print(f"Dinlenen port: {PORT}")

session = 0

while True:
    print("\nESP32 bekleniyor...")

    conn, addr = server.accept()
    print(f"Bağlantı: {addr}")

    session += 1
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    session_dir = SAVE_DIR / f"session_{session:04d}_{timestamp}"
    session_dir.mkdir(parents=True, exist_ok=True)

    frame_count = 0
    image_paths = []

    try:
        frame_count = struct.unpack("B", recv_exact(conn, 1))[0]
        print(f"{frame_count} kare bekleniyor")

        for i in range(frame_count):
            size = struct.unpack(">I", recv_exact(conn, 4))[0]
            jpg = recv_exact(conn, size)

            filename = session_dir / f"frame_{i+1:02d}.jpg"

            with open(filename, "wb") as f:
                f.write(jpg)

            image_paths.append(filename)

            print(
                f"  [{i+1}/{frame_count}] kaydedildi → "
                f"{filename.name} ({size} byte)"
            )

        print("\nModel çalışıyor...")
        result = analyze_burst(image_paths)

        print("\n========== DETAYLI MODEL SONUCU ==========")
        print(f"Karar                : {result['decision']}")
        print(f"Tespit var mı?       : {'EVET' if result['detected'] else 'HAYIR'}")
        print(f"Olasılık skoru       : %{result['probability_percent']}")
        print(
            f"Pozitif kare         : {result['positive_frames']} / {result['total_frames']}"
        )
        print(
            f"Negatif kare         : {result['negative_frames']} / {result['total_frames']}"
        )
        print(f"Pozitif oran         : {result['positive_ratio']}")
        print(f"Toplam tespit        : {result['total_detections']}")
        print(f"Maksimum confidence  : {result['max_confidence']}")
        print(f"En iyi kare          : {result['best_frame']}")
        print(f"Analiz süresi        : {result['elapsed_seconds']} sn")
        print(f"Karar sebebi         : {result['decision_reason']}")

        print("\nSınıf özeti:")
        if result["class_summary"]:
            for cls_name, info in result["class_summary"].items():
                print(
                    f"  - {cls_name}: {info['count']} tespit, "
                    f"max confidence: {info['max_confidence']}"
                )
        else:
            print("  - Hiç nesne tespit edilmedi.")

        print("\nKare bazlı özet:")
        for frame in result["frame_details"]:
            print(
                f"  Frame {frame['frame_index']:02d} | "
                f"{frame['filename']} | "
                f"Pozitif: {'EVET' if frame['is_positive'] else 'HAYIR'} | "
                f"Max conf: {frame['max_confidence']} | "
                f"Tespit sayısı: {frame['detection_count']}"
            )

        print("==========================================\n")

        result_file = session_dir / "result.txt"
        json_file = session_dir / "result.json"
        csv_file = session_dir / "frame_details.csv"

        with open(result_file, "w", encoding="utf-8") as f:
            f.write("DETAYLI MODEL SONUCU\n")
            f.write("====================\n\n")

            f.write(f"Karar               : {result['decision']}\n")
            f.write(
                f"Tespit var mı?      : {'EVET' if result['detected'] else 'HAYIR'}\n"
            )
            f.write(f"Olasılık skoru      : %{result['probability_percent']}\n")
            f.write(
                f"Pozitif kare        : {result['positive_frames']} / {result['total_frames']}\n"
            )
            f.write(
                f"Negatif kare        : {result['negative_frames']} / {result['total_frames']}\n"
            )
            f.write(f"Pozitif oran        : {result['positive_ratio']}\n")
            f.write(f"Toplam tespit       : {result['total_detections']}\n")
            f.write(f"Maksimum confidence : {result['max_confidence']}\n")
            f.write(f"En iyi kare         : {result['best_frame']}\n")
            f.write(f"Analiz süresi       : {result['elapsed_seconds']} sn\n")
            f.write(f"Karar sebebi        : {result['decision_reason']}\n\n")

            f.write("Eşik Değerleri\n")
            f.write("-------------\n")
            f.write(
                f"CONF_THRESHOLD        : {result['thresholds']['conf_threshold']}\n"
            )
            f.write(
                f"STRONG_CONF_THRESHOLD : {result['thresholds']['strong_conf_threshold']}\n"
            )
            f.write(
                f"MIN_POSITIVE_FRAMES   : {result['thresholds']['min_positive_frames']}\n"
            )
            f.write(f"IMG_SIZE              : {result['thresholds']['img_size']}\n\n")

            f.write("Sınıf Özeti\n")
            f.write("-----------\n")
            if result["class_summary"]:
                for cls_name, info in result["class_summary"].items():
                    f.write(
                        f"{cls_name}: {info['count']} tespit, "
                        f"max confidence: {info['max_confidence']}\n"
                    )
            else:
                f.write("Hiç nesne tespit edilmedi.\n")

            f.write("\nKare Bazlı Detay\n")
            f.write("----------------\n")
            for frame in result["frame_details"]:
                f.write(f"\nFrame {frame['frame_index']:02d} - {frame['filename']}\n")
                f.write(
                    f"Pozitif mi?      : {'EVET' if frame['is_positive'] else 'HAYIR'}\n"
                )
                f.write(f"Max confidence   : {frame['max_confidence']}\n")
                f.write(f"Tespit sayısı    : {frame['detection_count']}\n")

                if frame["detections"]:
                    for det in frame["detections"]:
                        f.write(
                            f"  - Nesne {det['detection_index']}: "
                            f"{det['class_name']} "
                            f"(class_id={det['class_id']}, "
                            f"conf={det['confidence']})\n"
                        )
                else:
                    f.write("  - Tespit yok.\n")

        with open(json_file, "w", encoding="utf-8") as f:
            json.dump(result, f, ensure_ascii=False, indent=4)

        with open(csv_file, "w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(
                [
                    "frame_index",
                    "filename",
                    "is_positive",
                    "max_confidence",
                    "detection_count",
                    "detections",
                ]
            )

            for frame in result["frame_details"]:
                detection_text = "; ".join(
                    [
                        f"{d['class_name']}:{d['confidence']}"
                        for d in frame["detections"]
                    ]
                )

                writer.writerow(
                    [
                        frame["frame_index"],
                        frame["filename"],
                        frame["is_positive"],
                        frame["max_confidence"],
                        frame["detection_count"],
                        detection_text,
                    ]
                )

        print(f"Sonuç TXT kaydedildi : {result_file}")
        print(f"Sonuç JSON kaydedildi: {json_file}")
        print(f"Kare CSV kaydedildi  : {csv_file}")

    except Exception as e:
        print(f"Hata: {e}")

    finally:
        conn.close()
        print(f"Oturum tamamlandı. Toplam {frame_count} kare.")
