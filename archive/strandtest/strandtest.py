import serial
import threading
import time
import tkinter as tk
from tkinter import messagebox

# ------------------------------------------------------------
# Einstellungen
# ------------------------------------------------------------

PORT = "COM15"
FPS = 30

MASTER_CHANNEL = 1
RED_CHANNEL = 2
GREEN_CHANNEL = 3
BLUE_CHANNEL = 4


# ------------------------------------------------------------
# DMX-Sender
# ------------------------------------------------------------

class DmxSender:
    def __init__(self, port):
        self.data = bytearray(513)
        self.data[0] = 0  # DMX-Startcode

        self.lock = threading.Lock()
        self.stop_event = threading.Event()
        self.error = None

        self.serial = serial.Serial(
            port=port,
            baudrate=250000,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_TWO,
            timeout=0,
            write_timeout=1
        )

        self.thread = threading.Thread(
            target=self._send_loop,
            daemon=True
        )

    def start(self):
        self.thread.start()

    def set_channel(self, channel, value):
        value = max(0, min(255, int(value)))

        with self.lock:
            self.data[channel] = value

    def set_rgb(self, master, red, green, blue):
        with self.lock:
            self.data[MASTER_CHANNEL] = master
            self.data[RED_CHANNEL] = red
            self.data[GREEN_CHANNEL] = green
            self.data[BLUE_CHANNEL] = blue

    def send_frame(self):
        with self.lock:
            frame = bytes(self.data)

        # DMX BREAK
        self.serial.break_condition = True
        time.sleep(0.001)

        # Mark After Break
        self.serial.break_condition = False
        time.sleep(0.001)

        # Startcode und 512 Kanäle
        self.serial.write(frame)
        self.serial.flush()

    def _send_loop(self):
        frame_duration = 1.0 / FPS

        try:
            while not self.stop_event.is_set():
                frame_start = time.monotonic()

                self.send_frame()

                elapsed = time.monotonic() - frame_start
                remaining = frame_duration - elapsed

                if remaining > 0:
                    self.stop_event.wait(remaining)

        except Exception as error:
            self.error = error
            self.stop_event.set()

    def close(self):
        self.stop_event.set()
        self.thread.join(timeout=1)

        # Vor dem Schließen Blackout senden
        self.set_rgb(0, 0, 0, 0)

        try:
            for _ in range(3):
                self.send_frame()
        except Exception:
            pass

        self.serial.close()


# ------------------------------------------------------------
# Benutzeroberfläche
# ------------------------------------------------------------

class DmxMixer:
    def __init__(self, root, sender):
        self.root = root
        self.sender = sender

        root.title("Python DMX-Mischpult")
        root.geometry("660x470")
        root.resizable(False, False)
        root.configure(bg="#202020")

        self.channels = [
            ("Master", MASTER_CHANNEL, "#dddddd"),
            ("Rot", RED_CHANNEL, "#ff5555"),
            ("Grün", GREEN_CHANNEL, "#55dd55"),
            ("Blau", BLUE_CHANNEL, "#5599ff")
        ]

        self.variables = {}

        title = tk.Label(
            root,
            text="DMX RGB-Mischpult",
            font=("Arial", 20, "bold"),
            fg="white",
            bg="#202020"
        )
        title.pack(pady=15)

        channel_frame = tk.Frame(root, bg="#202020")
        channel_frame.pack(padx=20, pady=5)

        for column, (name, channel, color) in enumerate(self.channels):
            self.create_fader(
                channel_frame,
                column,
                name,
                channel,
                color
            )

        button_frame = tk.Frame(root, bg="#202020")
        button_frame.pack(pady=20)

        tk.Button(
            button_frame,
            text="BLACKOUT",
            width=14,
            height=2,
            bg="#111111",
            fg="white",
            activebackground="#333333",
            activeforeground="white",
            command=self.blackout
        ).pack(side=tk.LEFT, padx=10)

        tk.Button(
            button_frame,
            text="VOLLES WEISS",
            width=14,
            height=2,
            bg="white",
            fg="black",
            command=self.full_white
        ).pack(side=tk.LEFT, padx=10)

        tk.Button(
            button_frame,
            text="ROT",
            width=10,
            height=2,
            bg="#cc3333",
            fg="white",
            command=lambda: self.set_values(255, 255, 0, 0)
        ).pack(side=tk.LEFT, padx=5)

        tk.Button(
            button_frame,
            text="GRÜN",
            width=10,
            height=2,
            bg="#33aa33",
            fg="white",
            command=lambda: self.set_values(255, 0, 255, 0)
        ).pack(side=tk.LEFT, padx=5)

        tk.Button(
            button_frame,
            text="BLAU",
            width=10,
            height=2,
            bg="#3366cc",
            fg="white",
            command=lambda: self.set_values(255, 0, 0, 255)
        ).pack(side=tk.LEFT, padx=5)

        self.status = tk.Label(
            root,
            text=f"DMX-Ausgabe: {PORT} · 250000 Baud · 8N2",
            fg="#66ff66",
            bg="#202020",
            font=("Consolas", 10)
        )
        self.status.pack(pady=5)

        root.protocol("WM_DELETE_WINDOW", self.close)
        root.after(500, self.check_sender)

    def create_fader(self, parent, column, name, channel, color):
        frame = tk.Frame(
            parent,
            bg="#303030",
            padx=15,
            pady=10
        )
        frame.grid(row=0, column=column, padx=8)

        tk.Label(
            frame,
            text=f"CH {channel}",
            font=("Arial", 10),
            fg="#aaaaaa",
            bg="#303030"
        ).pack()

        tk.Label(
            frame,
            text=name,
            font=("Arial", 13, "bold"),
            fg=color,
            bg="#303030"
        ).pack(pady=5)

        variable = tk.IntVar(value=0)
        self.variables[channel] = variable

        value_label = tk.Label(
            frame,
            text="0",
            width=4,
            font=("Consolas", 15, "bold"),
            fg="white",
            bg="#303030"
        )
        value_label.pack()

        slider = tk.Scale(
            frame,
            from_=255,
            to=0,
            variable=variable,
            length=260,
            width=28,
            sliderlength=25,
            showvalue=False,
            orient=tk.VERTICAL,
            bg="#303030",
            fg="white",
            troughcolor="#111111",
            activebackground=color,
            highlightthickness=0,
            command=lambda value, ch=channel, label=value_label:
                self.fader_changed(ch, value, label)
        )
        slider.pack()

    def fader_changed(self, channel, value, label):
        value = int(float(value))

        label.config(text=str(value))
        self.sender.set_channel(channel, value)

    def set_values(self, master, red, green, blue):
        values = {
            MASTER_CHANNEL: master,
            RED_CHANNEL: red,
            GREEN_CHANNEL: green,
            BLUE_CHANNEL: blue
        }

        for channel, value in values.items():
            self.variables[channel].set(value)
            self.sender.set_channel(channel, value)

    def blackout(self):
        self.set_values(0, 0, 0, 0)

    def full_white(self):
        self.set_values(255, 255, 255, 255)

    def check_sender(self):
        if self.sender.error is not None:
            self.status.config(
                text=f"DMX-Fehler: {self.sender.error}",
                fg="#ff5555"
            )
        else:
            self.root.after(500, self.check_sender)

    def close(self):
        self.status.config(text="Blackout und Port schließen …")
        self.root.update_idletasks()

        self.sender.close()
        self.root.destroy()


# ------------------------------------------------------------
# Programmstart
# ------------------------------------------------------------

def main():
    try:
        sender = DmxSender(PORT)

    except serial.SerialException as error:
        root = tk.Tk()
        root.withdraw()

        messagebox.showerror(
            "Serieller Port",
            f"{PORT} konnte nicht geöffnet werden:\n\n{error}"
        )

        root.destroy()
        return

    sender.start()

    root = tk.Tk()
    DmxMixer(root, sender)
    root.mainloop()


if __name__ == "__main__":
    main()
