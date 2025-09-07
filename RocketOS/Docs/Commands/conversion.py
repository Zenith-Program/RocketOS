import json
import os

def flatten_commands(data, path="", level=2):
    """
    Recursively flatten commands into Markdown format.
    - data: current node (dict or string)
    - path: full command path so far
    - level: heading level for Markdown
    """
    md_lines = []

    if isinstance(data, str):
        # Leaf command (just a string description)
        heading = "##" + f" {path.strip()}"
        md_lines.append(heading)
        md_lines.append(data.replace("\\n", "\n"))
        md_lines.append("")  # blank line
        return md_lines

    if isinstance(data, dict):
        # If this node has a "description" and maybe "children"
        description = data.get("description")
        children = data.get("children")

        if description is not None:
            heading = "##" + f" {path.strip()}"
            md_lines.append(heading)
            md_lines.append(description.replace("\\n", "\n"))
            md_lines.append("")

        if children is not None:
            for child_name, child_val in children.items():
                child_path = f"{path} {child_name}".strip()
                md_lines.extend(flatten_commands(child_val, child_path, level + 1))
        else:
            # Dictionary with just descriptions of subcommands
            for key, val in data.items():
                child_path = f"{path} {key}".strip()
                md_lines.extend(flatten_commands(val, child_path, level + 1))

    return md_lines


def json_to_markdown(json_file, output_file):
    with open(json_file, "r") as f:
        data = json.load(f)

    md_lines = []
    for cmd, content in data.items():
        md_lines.extend(flatten_commands(content, cmd, 2))

    with open(output_file, "w") as f:
        f.write("\n".join(md_lines))


if __name__ == "__main__":
    CONFIG_FILE_NAME = "data.json"
    OUTPUT_FILE_NAME = "converted.md"
    SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
    CONFIG_FILE = os.path.join(SCRIPT_DIR, CONFIG_FILE_NAME)
    OUTPUT_FILE = os.path.join(SCRIPT_DIR, OUTPUT_FILE_NAME)

    json_to_markdown(CONFIG_FILE, OUTPUT_FILE)
    print(f"✅ Markdown file generated: {OUTPUT_FILE}")