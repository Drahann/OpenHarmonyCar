import argparse
import os
import tkinter as tk


DEFAULT_LOGS = ["navi.log", "udp2lcm.log", "serial.log", "lidar.log"]


def read_robots(config_path):
    robots = []
    with open(config_path, "r", encoding="utf-8") as cfg:
        for line in cfg:
            item = line.strip()
            if item and not item.startswith("#"):
                robots.append(item)
    return robots


def safe_name(robot):
    return "".join(ch if ch not in ':\\/*?"<>|' else "_" for ch in robot)


class LogViewer:
    def __init__(self, root, robots, log_root, log_names):
        self.root = root
        self.robots = robots
        self.log_root = log_root
        self.log_names = log_names
        self.current_robot = robots[0]
        self.current_log = log_names[0]
        self.paused = False

        self.root.title("Robot logs")
        self.root.geometry("1100x700")

        self.robot_var = tk.StringVar(value=self.current_robot)
        self.log_var = tk.StringVar(value=self.current_log)

        left = tk.Frame(root, width=180)
        left.pack(side=tk.LEFT, fill=tk.Y, padx=10, pady=10)
        left.pack_propagate(False)

        tk.Label(left, text="Robot").pack(anchor="w")
        for robot in robots:
            tk.Radiobutton(
                left,
                text=robot,
                variable=self.robot_var,
                value=robot,
                command=self.choose_robot,
            ).pack(anchor="w")

        tk.Label(left, text="Log").pack(anchor="w", pady=(16, 0))
        for log_name in log_names:
            tk.Radiobutton(
                left,
                text=log_name,
                variable=self.log_var,
                value=log_name,
                command=self.choose_log,
            ).pack(anchor="w")

        tk.Button(left, text="Pause / Resume", command=self.toggle_pause).pack(
            fill=tk.X, pady=(24, 8)
        )
        tk.Button(left, text="Exit", command=root.destroy).pack(fill=tk.X)

        right = tk.Frame(root)
        right.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=(0, 10), pady=10)

        self.status = tk.Label(right, text="", anchor="w")
        self.status.pack(fill=tk.X)

        self.text = tk.Text(right, wrap=tk.NONE, font=("Consolas", 11))
        self.text.pack(fill=tk.BOTH, expand=True)

        self.update_view()

    def choose_robot(self):
        self.current_robot = self.robot_var.get()
        self.paused = False

    def choose_log(self):
        self.current_log = self.log_var.get()
        self.paused = False

    def toggle_pause(self):
        self.paused = not self.paused

    def update_view(self):
        path = os.path.join(self.log_root, safe_name(self.current_robot), self.current_log)
        suffix = " (paused)" if self.paused else ""
        self.status.config(text=f"{self.current_robot} -> {self.current_log}{suffix}")

        if not self.paused:
            if os.path.exists(path):
                try:
                    with open(path, "r", encoding="utf-8", errors="replace") as log_file:
                        content = log_file.read()
                except OSError as exc:
                    content = f"Cannot read log: {exc}"
            else:
                content = f"Waiting for log file:\n{path}"

            self.text.delete("1.0", tk.END)
            self.text.insert(tk.END, content)
            self.text.see(tk.END)

        self.root.after(500, self.update_view)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", required=True)
    parser.add_argument("--log-root", required=True)
    parser.add_argument("--logs", nargs="+", default=DEFAULT_LOGS)
    args = parser.parse_args()

    robots = read_robots(args.config)
    if not robots:
        raise SystemExit(f"No robot IP found in {args.config}")

    os.makedirs(args.log_root, exist_ok=True)
    for robot in robots:
        os.makedirs(os.path.join(args.log_root, safe_name(robot)), exist_ok=True)

    root = tk.Tk()
    LogViewer(root, robots, args.log_root, args.logs)
    root.mainloop()


if __name__ == "__main__":
    main()
