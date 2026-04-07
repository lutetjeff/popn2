import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import serial
import serial.tools.list_ports
import re

# HID Hex Map
HID_KEYS = {
    "A": "0x04", "B": "0x05", "C": "0x06", "D": "0x07", "E": "0x08", "F": "0x09",
    "G": "0x0A", "H": "0x0B", "I": "0x0C", "J": "0x0D", "K": "0x0E", "L": "0x0F",
    "M": "0x10", "N": "0x11", "O": "0x12", "P": "0x13", "Q": "0x14", "R": "0x15",
    "S": "0x16", "T": "0x17", "U": "0x18", "V": "0x19", "W": "0x1A", "X": "0x1B",
    "Y": "0x1C", "Z": "0x1D",
    "1": "0x1E", "2": "0x1F", "3": "0x20", "4": "0x21", "5": "0x22", "6": "0x23",
    "7": "0x24", "8": "0x25", "9": "0x26", "0": "0x27",
    "Enter": "0x28", "Escape": "0x29", "Backspace": "0x2A", "Tab": "0x2B",
    "Spacebar": "0x2C", "Minus (-)": "0x2D", "Equals (=)": "0x2E"
}

BUTTONS = [f"B{i}" for i in range(12)]

class ControllerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("STM32 Controller Configurator")
        self.root.geometry("680x600") # Made slightly taller for the new panel
        
        self.serial_conn = None
        self.rx_buffer = ""      # Used to assemble incoming serial characters into full lines
        self.is_polling = False  # State of the live read toggle
        
        self.setup_ui()
        self.refresh_ports()

    def setup_ui(self):
        # --- Top Frame: Connection ---
        conn_frame = tk.Frame(self.root, padx=10, pady=5)
        conn_frame.pack(fill=tk.X)

        tk.Label(conn_frame, text="COM Port:").pack(side=tk.LEFT)
        self.port_cb = ttk.Combobox(conn_frame, state="readonly", width=15)
        self.port_cb.pack(side=tk.LEFT, padx=5)
        
        tk.Button(conn_frame, text="Refresh", command=self.refresh_ports).pack(side=tk.LEFT)
        self.btn_connect = tk.Button(conn_frame, text="Connect", command=self.toggle_connection, width=12)
        self.btn_connect.pack(side=tk.LEFT, padx=10)

        # --- Main Content Split ---
        main_frame = tk.Frame(self.root, padx=10, pady=5)
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        controls_frame = tk.Frame(main_frame)
        controls_frame.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 10))

        terminal_frame = tk.Frame(main_frame)
        terminal_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        # === CONTROLS SECTION ===

        # 1. Global Settings
        lf_global = tk.LabelFrame(controls_frame, text="Global Settings", padx=10, pady=10)
        lf_global.pack(fill=tk.X, pady=5)

        tk.Label(lf_global, text="Debounce (ms):").grid(row=0, column=0, sticky="w")
        self.debounce_var = tk.StringVar(value="10")
        tk.Entry(lf_global, textvariable=self.debounce_var, width=5).grid(row=0, column=1, padx=5)
        tk.Button(lf_global, text="Set", command=self.cmd_set_debounce).grid(row=0, column=2)

        tk.Label(lf_global, text="Global LEDs:").grid(row=1, column=0, sticky="w", pady=(5, 0))
        tk.Button(lf_global, text="Enable", command=lambda: self.send_cmd("e 1")).grid(row=1, column=1, pady=(5,0))
        tk.Button(lf_global, text="Disable", command=lambda: self.send_cmd("e 0")).grid(row=1, column=2, pady=(5,0))

        # 2. Button Configuration
        lf_btn = tk.LabelFrame(controls_frame, text="Button Configuration", padx=10, pady=10)
        lf_btn.pack(fill=tk.X, pady=5)

        tk.Label(lf_btn, text="Target Button:").grid(row=0, column=0, sticky="w")
        self.btn_sel_config = ttk.Combobox(lf_btn, values=BUTTONS, state="readonly", width=5)
        self.btn_sel_config.current(0)
        self.btn_sel_config.grid(row=0, column=1, columnspan=2, sticky="w", padx=5)

        tk.Label(lf_btn, text="Key Mapping:").grid(row=1, column=0, sticky="w", pady=(10, 0))
        self.key_cb = ttk.Combobox(lf_btn, values=list(HID_KEYS.keys()), state="readonly", width=10)
        self.key_cb.current(0)
        self.key_cb.grid(row=1, column=1, padx=5, pady=(10, 0))
        tk.Button(lf_btn, text="Apply", command=self.cmd_set_key).grid(row=1, column=2, pady=(10, 0))

        tk.Label(lf_btn, text="LED Mode:").grid(row=2, column=0, sticky="w", pady=(5, 0))
        self.mode_cb = ttk.Combobox(lf_btn, values=["0: Direct (Hardware)", "1: COM Controlled"], state="readonly", width=16)
        self.mode_cb.current(0)
        self.mode_cb.grid(row=2, column=1, columnspan=2, padx=5, pady=(5, 0), sticky="w")
        tk.Button(lf_btn, text="Apply", command=self.cmd_set_mode).grid(row=3, column=1, columnspan=2, pady=(5, 0), sticky="w", padx=5)


        # 3. LIVE HARDWARE DASHBOARD
        lf_live = tk.LabelFrame(controls_frame, text="Live Hardware State", padx=10, pady=10)
        lf_live.pack(fill=tk.X, pady=5)

        poll_ctrl_frame = tk.Frame(lf_live)
        poll_ctrl_frame.pack(fill=tk.X, pady=(0, 10))
        
        tk.Label(poll_ctrl_frame, text="Interval (ms):").pack(side=tk.LEFT)
        self.poll_interval_var = tk.StringVar(value="100")
        tk.Entry(poll_ctrl_frame, textvariable=self.poll_interval_var, width=5).pack(side=tk.LEFT, padx=5)
        self.btn_poll = tk.Button(poll_ctrl_frame, text="▶ Start Polling", command=self.toggle_polling, bg="#e0ffe0")
        self.btn_poll.pack(side=tk.LEFT, padx=5)

        # Create a grid of 12 indicator blocks
        self.btn_indicators = []
        ind_frame = tk.Frame(lf_live)
        ind_frame.pack(fill=tk.X)
        
        for i in range(12):
            lbl = tk.Label(ind_frame, text=f"B{i}", width=4, height=2, bg="#e0e0e0", relief=tk.RAISED)
            # Arrange in 3 rows of 4 columns
            lbl.grid(row=i//4, column=i%4, padx=3, pady=3)
            self.btn_indicators.append(lbl)

        # === TERMINAL SECTION ===
        tk.Label(terminal_frame, text="Serial Output Log:").pack(anchor="w")
        self.log_text = scrolledtext.ScrolledText(terminal_frame, wrap=tk.WORD, state='disabled')
        self.log_text.pack(fill=tk.BOTH, expand=True)

        # === FOOTER SECTION ===
        footer_frame = tk.Frame(self.root, padx=10, pady=10)
        footer_frame.pack(fill=tk.X, side=tk.BOTTOM)
        
        tk.Button(footer_frame, text="💾 Save Current Config to Flash", bg="#e0ffe0", height=2, command=lambda: self.send_cmd("S")).pack(side=tk.LEFT, expand=True, fill=tk.X, padx=(0, 5))
        tk.Button(footer_frame, text="⚠️ Reboot to DFU Bootloader", bg="#ffe0e0", height=2, command=lambda: self.send_cmd("Z")).pack(side=tk.RIGHT, expand=True, fill=tk.X, padx=(5, 0))

    # --- Hardware Command Generators ---
    def cmd_set_debounce(self):
        val = self.debounce_var.get()
        if val.isdigit(): self.send_cmd(f"t {val}")
        else: messagebox.showwarning("Input Error", "Debounce must be an integer.")

    def cmd_set_key(self):
        btn_idx = self.btn_sel_config.current()
        hex_val = HID_KEYS[self.key_cb.get()]
        self.send_cmd(f"k {btn_idx} {hex_val}")

    def cmd_set_mode(self):
        btn_idx = self.btn_sel_config.current()
        mode_idx = 0 if "0:" in self.mode_cb.get() else 1
        self.send_cmd(f"m {btn_idx} {mode_idx}")

    # --- Live Polling Logic ---
    def toggle_polling(self):
        if self.is_polling:
            self.is_polling = False
            self.btn_poll.config(text="▶ Start Polling", bg="#e0ffe0")
            # Reset colors to grey when stopped
            for lbl in self.btn_indicators:
                lbl.config(bg="#e0e0e0", relief=tk.RAISED)
        else:
            if not self.serial_conn or not self.serial_conn.is_open:
                messagebox.showwarning("Warning", "Connect to the controller first!")
                return
            self.is_polling = True
            self.btn_poll.config(text="⏹ Stop Polling", bg="#ffe0e0")
            self.poll_buttons()

    def poll_buttons(self):
        if not self.is_polling or not self.serial_conn or not self.serial_conn.is_open:
            self.is_polling = False
            self.btn_poll.config(text="▶ Start Polling", bg="#e0ffe0")
            return
            
        # Quietly send the read command for all 12 buttons
        try:
            for i in range(12):
                self.serial_conn.write(f"r {i}\n".encode('utf-8'))
        except:
            pass
            
        # Schedule the next poll
        try: interval = int(self.poll_interval_var.get())
        except ValueError:
            interval = 100
            self.poll_interval_var.set("100")
            
        self.root.after(interval, self.poll_buttons)

    # --- Serial Communication Logic ---
    def refresh_ports(self):
        ports = serial.tools.list_ports.comports()
        self.port_cb['values'] = [port.device for port in ports]
        if self.port_cb['values']: self.port_cb.current(0)

    def toggle_connection(self):
        if self.serial_conn and self.serial_conn.is_open:
            self.serial_conn.close()
            self.btn_connect.config(text="Connect")
            self.is_polling = False
            self.btn_poll.config(text="▶ Start Polling", bg="#e0ffe0")
            self.log_msg("--- Disconnected ---\n")
        else:
            port = self.port_cb.get()
            if not port: return
            try:
                self.serial_conn = serial.Serial(port, baudrate=115200, timeout=0.1)
                self.btn_connect.config(text="Disconnect")
                self.log_msg(f"--- Connected to {port} ---\n")
                self.root.after(50, self.read_serial)
            except Exception as e:
                messagebox.showerror("Connection Error", str(e))

    def read_serial(self):
        if self.serial_conn and self.serial_conn.is_open:
            try:
                if self.serial_conn.in_waiting > 0:
                    data = self.serial_conn.read(self.serial_conn.in_waiting).decode('utf-8', errors='ignore')
                    self.rx_buffer += data
                    
                    # Process completed lines
                    while '\n' in self.rx_buffer:
                        line, self.rx_buffer = self.rx_buffer.split('\n', 1)
                        line = line.strip()
                        if not line: continue
                        
                        # 1. Intercept Button Read Responses
                        if line.startswith("READ "):
                            try:
                                parts = line.replace("READ ", "").split(":")
                                btn_idx = int(parts[0].strip())
                                state = int(parts[1].strip())
                                
                                # Visual update: Green/Sunken if pressed, Grey/Raised if released
                                if state == 1:
                                    self.btn_indicators[btn_idx].config(bg="#4caf50", relief=tk.SUNKEN)
                                else:
                                    self.btn_indicators[btn_idx].config(bg="#e0e0e0", relief=tk.RAISED)
                            except:
                                pass
                                
                        # 2. Silently ignore the echoes of the polling commands
                        elif re.match(r"^r \d+$", line) and self.is_polling:
                            pass 
                            
                        # 3. Log everything else (like "OK" or custom commands)
                        else:
                            self.log_msg(line + "\n")
                            
                self.root.after(50, self.read_serial)
            except Exception as e:
                self.log_msg(f"\n--- Connection Lost: {str(e)} ---\n")
                self.toggle_connection()

    def send_cmd(self, cmd_string):
        if self.serial_conn and self.serial_conn.is_open:
            self.log_msg(f"TX: {cmd_string}\n")
            try:
                full_cmd = cmd_string + "\n"
                self.serial_conn.write(full_cmd.encode('utf-8'))
            except Exception as e:
                messagebox.showerror("Send Error", str(e))
        else:
            messagebox.showwarning("Warning", "Connect to the controller first!")

    def log_msg(self, msg):
        self.log_text.config(state='normal')
        self.log_text.insert(tk.END, msg)
        self.log_text.see(tk.END)
        self.log_text.config(state='disabled')

if __name__ == "__main__":
    root = tk.Tk()
    app = ControllerApp(root)
    root.mainloop()