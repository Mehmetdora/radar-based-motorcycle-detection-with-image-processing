import json
import zipfile
from pathlib import Path
from datetime import datetime

import pandas as pd
import streamlit as st


DEFAULT_SAVE_DIR = Path.home() / "Desktop" / "esp32_photos"


st.set_page_config(
    page_title="Sidewalk Motorcycle Detection Dashboard",
    layout="wide",
)


def load_json(json_path: Path) -> dict:
    try:
        with open(json_path, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception as e:
        return {"error": str(e)}


def load_text(text_path: Path) -> str:
    try:
        with open(text_path, "r", encoding="utf-8") as f:
            return f.read()
    except Exception as e:
        return f"Log file could not be read: {e}"


def load_csv(csv_path: Path) -> pd.DataFrame:
    try:
        return pd.read_csv(csv_path)
    except Exception:
        return pd.DataFrame()


def safe_get(data: dict, key: str, default="-"):
    value = data.get(key, default)
    if value is None:
        return default
    return value


def format_bool(value):
    if value is True:
        return "Yes"
    if value is False:
        return "No"
    return "-"


def find_sessions(base_dir: Path):
    if not base_dir.exists():
        return []

    sessions = []

    for folder in base_dir.iterdir():
        if folder.is_dir() and folder.name.startswith("session_"):
            result_json = folder / "result.json"
            result_txt = folder / "result.txt"
            frame_csv = folder / "frame_details.csv"

            if result_json.exists():
                result = load_json(result_json)
            else:
                result = {}

            sessions.append(
                {
                    "folder": folder,
                    "name": folder.name,
                    "result_json": result_json,
                    "result_txt": result_txt,
                    "frame_csv": frame_csv,
                    "result": result,
                    "modified_time": folder.stat().st_mtime,
                }
            )

    sessions.sort(key=lambda x: x["modified_time"], reverse=True)
    return sessions


def get_image_files(session_folder: Path):
    files = []
    for ext in ["*.jpg", "*.jpeg", "*.png"]:
        files.extend(session_folder.glob(ext))
    files.sort()
    return files


def create_session_zip(session_folder: Path) -> bytes:
    zip_path = session_folder.parent / f"{session_folder.name}.zip"

    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zipf:
        for file_path in session_folder.rglob("*"):
            if file_path.is_file():
                zipf.write(file_path, arcname=file_path.relative_to(session_folder))

    with open(zip_path, "rb") as f:
        data = f.read()

    try:
        zip_path.unlink()
    except Exception:
        pass

    return data


def build_summary_dataframe(sessions):
    rows = []

    for session in sessions:
        result = session["result"]
        folder = session["folder"]

        rows.append(
            {
                "Session": session["name"],
                "Decision": safe_get(result, "decision"),
                "Detected": format_bool(result.get("detected")),
                "Probability (%)": safe_get(result, "probability_percent", 0),
                "Positive Frames": safe_get(result, "positive_frames", 0),
                "Total Frames": safe_get(result, "total_frames", 0),
                "Positive Ratio": safe_get(result, "positive_ratio", 0),
                "Max Confidence": safe_get(result, "max_confidence", 0),
                "Total Detections": safe_get(result, "total_detections", 0),
                "STM32 Final": format_bool(result.get("radar_final_detected")),
                "STM32 Obj": safe_get(result, "first_stage_decision"),
                "Confirm Count": safe_get(result, "radar_confirm_count", 0),
                "Speed (km/h)": safe_get(result, "estimated_speed_kmh", 0),
                "Freq (Hz)": safe_get(result, "dominant_doppler_frequency_hz", 0),
                "Power": safe_get(result, "radar_signal_power", 0),
                "Fusion": safe_get(result, "fusion_decision"),
                "Elapsed Time (s)": safe_get(result, "elapsed_seconds", 0),
                "Folder": str(folder),
            }
        )

    return pd.DataFrame(rows)


def extract_datetime_from_session_name(session_name: str):
    try:
        parts = session_name.split("_")
        date_part = parts[-2]
        time_part = parts[-1]
        dt = datetime.strptime(date_part + "_" + time_part, "%Y%m%d_%H%M%S")
        return dt.strftime("%Y-%m-%d %H:%M:%S")
    except Exception:
        return "-"


st.sidebar.title("Dashboard Settings")

base_dir_input = st.sidebar.text_input(
    "Log folder",
    value=str(DEFAULT_SAVE_DIR),
)

base_dir = Path(base_dir_input).expanduser()

if st.sidebar.button("Refresh Data", key="refresh_data_button"):
    st.rerun()


st.title("Radar-Triggered Sidewalk Motorcycle Detection Dashboard")

st.markdown(
    """
This dashboard reads the event folders generated by the Python server.
It displays YOLO validation results, captured images, frame-level details,
log files, and downloadable event outputs.
"""
)

st.caption(f"Active log folder: {base_dir}")


sessions = find_sessions(base_dir)

if not base_dir.exists():
    st.error("The selected log folder does not exist.")
    st.stop()

if not sessions:
    st.warning("No session folder was found. Run server.py and receive images from ESP32 first.")
    st.stop()


summary_df = build_summary_dataframe(sessions)

total_events = len(sessions)
confirmed_events = sum(1 for s in sessions if s["result"].get("detected") is True)
rejected_events = sum(1 for s in sessions if s["result"].get("detected") is False)

avg_probability = (
    summary_df["Probability (%)"].replace("-", 0).astype(float).mean()
    if not summary_df.empty
    else 0
)

avg_confidence = (
    summary_df["Max Confidence"].replace("-", 0).astype(float).mean()
    if not summary_df.empty
    else 0
)


col1, col2, col3, col4, col5 = st.columns(5)

with col1:
    st.metric("Total Events", total_events)

with col2:
    st.metric("Confirmed Events", confirmed_events)

with col3:
    st.metric("Rejected Events", rejected_events)

with col4:
    st.metric("Average Probability", f"{avg_probability:.2f}%")

with col5:
    st.metric("Average Max Confidence", f"{avg_confidence:.3f}")


st.divider()
st.subheader("Event List and Filters")

filter_col1, filter_col2, filter_col3 = st.columns(3)

with filter_col1:
    status_filter = st.selectbox(
        "Event status",
        ["All", "Only confirmed", "Only rejected"],
    )

with filter_col2:
    min_conf = st.slider(
        "Minimum max confidence",
        min_value=0.0,
        max_value=1.0,
        value=0.0,
        step=0.05,
    )

with filter_col3:
    min_positive_frames = st.number_input(
        "Minimum positive frames",
        min_value=0,
        max_value=1000,
        value=0,
        step=1,
    )


filtered_sessions = []

for session in sessions:
    result = session["result"]

    detected = result.get("detected")
    max_conf = float(result.get("max_confidence", 0) or 0)
    positive_frames = int(result.get("positive_frames", 0) or 0)

    if status_filter == "Only confirmed" and detected is not True:
        continue

    if status_filter == "Only rejected" and detected is not False:
        continue

    if max_conf < min_conf:
        continue

    if positive_frames < min_positive_frames:
        continue

    filtered_sessions.append(session)


filtered_summary_df = build_summary_dataframe(filtered_sessions)

st.dataframe(
    filtered_summary_df,
    use_container_width=True,
    hide_index=True,
)


st.divider()
st.subheader("Event Details")

session_names = [s["name"] for s in filtered_sessions]

if not session_names:
    st.warning("No event matches the selected filters.")
    st.stop()

selected_session_name = st.selectbox(
    "Select an event/session",
    session_names,
)

selected_session = next(
    s for s in filtered_sessions if s["name"] == selected_session_name
)

session_folder = selected_session["folder"]
result = selected_session["result"]
result_txt_path = selected_session["result_txt"]
result_json_path = selected_session["result_json"]
frame_csv_path = selected_session["frame_csv"]

image_files = get_image_files(session_folder)


st.markdown(f"### Selected Session: {selected_session_name}")

detail_col1, detail_col2, detail_col3, detail_col4 = st.columns(4)

with detail_col1:
    st.metric("Decision", safe_get(result, "decision"))

with detail_col2:
    st.metric("Detected", format_bool(result.get("detected")))

with detail_col3:
    st.metric("Probability Score", f"{safe_get(result, 'probability_percent', 0)}%")

with detail_col4:
    st.metric("Max Confidence", safe_get(result, "max_confidence", 0))


detail_col5, detail_col6, detail_col7, detail_col8 = st.columns(4)

with detail_col5:
    st.metric(
        "Positive Frames",
        f"{safe_get(result, 'positive_frames', 0)} / {safe_get(result, 'total_frames', 0)}",
    )

with detail_col6:
    st.metric("Positive Ratio", safe_get(result, "positive_ratio", 0))

with detail_col7:
    st.metric("Total Detections", safe_get(result, "total_detections", 0))

with detail_col8:
    st.metric("Analysis Time", f"{safe_get(result, 'elapsed_seconds', 0)} s")


st.markdown("#### Decision Reason")
st.info(safe_get(result, "decision_reason", "Decision reason not found."))

st.markdown("#### Event Time")
st.write(f"Session timestamp: **{extract_datetime_from_session_name(selected_session_name)}**")

st.markdown("#### STM32 FINAL Radar Trigger")
radar_col1, radar_col2, radar_col3, radar_col4 = st.columns(4)
with radar_col1:
    st.metric("Final", format_bool(result.get("radar_final_detected")))
with radar_col2:
    st.metric("Object", safe_get(result, "first_stage_decision"))
with radar_col3:
    st.metric("Confirm Count", safe_get(result, "radar_confirm_count", 0))
with radar_col4:
    st.metric("Motion", format_bool(result.get("radar_motion_detected")))

radar_col5, radar_col6, radar_col7, radar_col8 = st.columns(4)
with radar_col5:
    st.metric("Power", safe_get(result, "radar_signal_power", 0))
with radar_col6:
    st.metric("Freq Hz", safe_get(result, "dominant_doppler_frequency_hz", 0))
with radar_col7:
    st.metric("Speed km/h", safe_get(result, "estimated_speed_kmh", 0))
with radar_col8:
    st.metric("Fusion", safe_get(result, "fusion_decision"))

raw_radar_message = result.get("radar_raw_message")
if raw_radar_message:
    st.code(raw_radar_message, language="text")


st.divider()
st.subheader("Best Frame and Event Evidence")

best_frame_path = result.get("best_frame")

best_frame_col, file_info_col = st.columns([1, 1])

with best_frame_col:
    if best_frame_path and Path(best_frame_path).exists():
        st.image(
            best_frame_path,
            caption="Best frame with highest confidence",
            use_container_width=True,
        )
    else:
        st.warning("Best frame was not found or the file path is invalid.")

with file_info_col:
    st.markdown("#### Event File Information")
    st.write(f"Session folder: `{session_folder}`")
    st.write(f"Number of image files: **{len(image_files)}**")
    st.write(f"result.txt: {'Available' if result_txt_path.exists() else 'Missing'}")
    st.write(f"result.json: {'Available' if result_json_path.exists() else 'Missing'}")
    st.write(f"frame_details.csv: {'Available' if frame_csv_path.exists() else 'Missing'}")


st.divider()
st.subheader("Class Summary and Thresholds")

summary_col, threshold_col = st.columns(2)

with summary_col:
    st.markdown("#### Class Summary")

    class_summary = result.get("class_summary", {})

    if class_summary:
        class_rows = []
        for cls_name, info in class_summary.items():
            class_rows.append(
                {
                    "Class Name": cls_name,
                    "Count": info.get("count", 0),
                    "Max Confidence": info.get("max_confidence", 0),
                }
            )

        st.dataframe(
            pd.DataFrame(class_rows),
            use_container_width=True,
            hide_index=True,
        )
    else:
        st.info("No class summary was found for this event.")

with threshold_col:
    st.markdown("#### Threshold Values")

    thresholds = result.get("thresholds", {})

    if thresholds:
        threshold_rows = [{"Parameter": k, "Value": v} for k, v in thresholds.items()]
        st.dataframe(
            pd.DataFrame(threshold_rows),
            use_container_width=True,
            hide_index=True,
        )
    else:
        st.info("Threshold information was not found.")


st.divider()
st.subheader("Frame-Level Details")

frame_df = load_csv(frame_csv_path)

if not frame_df.empty:
    st.dataframe(
        frame_df,
        use_container_width=True,
        hide_index=True,
    )

    csv_bytes = frame_df.to_csv(index=False).encode("utf-8")

    st.download_button(
        label="Download frame_details.csv",
        data=csv_bytes,
        file_name=f"{selected_session_name}_frame_details.csv",
        mime="text/csv",
        key=f"download_frame_details_top_{selected_session_name}",
    )
else:
    st.warning("frame_details.csv could not be read or is empty.")


st.divider()
st.subheader("Frame Confidence Chart")

frame_scores = result.get("frame_scores", [])

if frame_scores:
    score_df = pd.DataFrame(
        {
            "Frame": list(range(1, len(frame_scores) + 1)),
            "Max Confidence": frame_scores,
        }
    )

    st.line_chart(
        data=score_df,
        x="Frame",
        y="Max Confidence",
        use_container_width=True,
    )
else:
    st.info("frame_scores information was not found.")


st.divider()
st.subheader("Image Gallery")

show_gallery = st.checkbox("Show all frame images", value=False)

if show_gallery:
    if image_files:
        gallery_cols = st.columns(4)

        for idx, img_path in enumerate(image_files):
            with gallery_cols[idx % 4]:
                st.image(
                    str(img_path),
                    caption=img_path.name,
                    use_container_width=True,
                )
    else:
        st.warning("No image files were found for this session.")


st.divider()
st.subheader("Session File List")

all_files = sorted([p for p in session_folder.rglob("*") if p.is_file()])

file_rows = []

for file_path in all_files:
    file_rows.append(
        {
            "File Name": file_path.name,
            "Relative Path": str(file_path.relative_to(session_folder)),
            "Size (KB)": round(file_path.stat().st_size / 1024, 2),
            "Extension": file_path.suffix,
        }
    )

file_df = pd.DataFrame(file_rows)

st.dataframe(
    file_df,
    use_container_width=True,
    hide_index=True,
)


st.divider()
st.subheader("Log Files and Downloads")

download_col1, download_col2, download_col3, download_col4 = st.columns(4)

with download_col1:
    if result_txt_path.exists():
        st.download_button(
            label="Download result.txt",
            data=result_txt_path.read_bytes(),
            file_name=f"{selected_session_name}_result.txt",
            mime="text/plain",
            key=f"download_result_txt_{selected_session_name}",
        )

with download_col2:
    if result_json_path.exists():
        st.download_button(
            label="Download result.json",
            data=result_json_path.read_bytes(),
            file_name=f"{selected_session_name}_result.json",
            mime="application/json",
            key=f"download_result_json_{selected_session_name}",
        )

with download_col3:
    if frame_csv_path.exists():
        st.download_button(
            label="Download frame_details.csv",
            data=frame_csv_path.read_bytes(),
            file_name=f"{selected_session_name}_frame_details.csv",
            mime="text/csv",
            key=f"download_frame_details_bottom_{selected_session_name}",
        )

with download_col4:
    zip_data = create_session_zip(session_folder)

    st.download_button(
        label="Download full session ZIP",
        data=zip_data,
        file_name=f"{selected_session_name}.zip",
        mime="application/zip",
        key=f"download_full_zip_{selected_session_name}",
    )


tab1, tab2, tab3 = st.tabs(
    ["result.txt", "result.json", "frame_details.csv"]
)

with tab1:
    if result_txt_path.exists():
        st.text_area(
            "result.txt content",
            value=load_text(result_txt_path),
            height=500,
            key=f"text_area_result_txt_{selected_session_name}",
        )
    else:
        st.warning("result.txt was not found.")

with tab2:
    if result_json_path.exists():
        st.json(result)
    else:
        st.warning("result.json was not found.")

with tab3:
    if not frame_df.empty:
        st.dataframe(
            frame_df,
            use_container_width=True,
            hide_index=True,
        )
    else:
        st.warning("frame_details.csv was not found or could not be read.")


st.divider()
st.subheader("STM32 Radar Metadata")

radar_placeholder = {
    "stm32_detection_time": result.get("stm32_detection_time", "Not available"),
    "radar_final_detected": result.get("radar_final_detected", "Not available"),
    "radar_confirm_count": result.get("radar_confirm_count", "Not available"),
    "first_stage_decision": result.get("first_stage_decision", "Not available"),
    "radar_motion_detected": result.get("radar_motion_detected", "Not available"),
    "radar_signal_power": result.get("radar_signal_power", "Not available"),
    "dominant_doppler_frequency_hz": result.get(
        "dominant_doppler_frequency_hz", "Not available"
    ),
    "estimated_speed_kmh": result.get("estimated_speed_kmh", "Not available"),
    "stm32_final_motor_trigger": result.get("stm32_final_motor_trigger", "Not available"),
    "fusion_detected": result.get("fusion_detected", "Not available"),
    "fusion_decision": result.get("fusion_decision", "Not available"),
    "radar_raw_message": result.get("radar_raw_message", "Not available"),
    "radar_metadata": result.get("radar_metadata", {}),
}

st.json(radar_placeholder)

radar_metadata = result.get("radar_metadata")
if radar_metadata:
    st.caption("Raw metadata received from ESP32.")
    st.json(radar_metadata)
else:
    st.info("This session does not contain STM32 radar metadata.")

st.divider()
st.caption(
    "This dashboard reads server-generated session folders, YOLO result files, frame details, and captured images."
)
