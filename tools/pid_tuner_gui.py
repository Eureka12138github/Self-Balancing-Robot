#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
平衡车 PID 参数实时调整工具

功能：
- 滑动条调节 Kp/Ki/Kd（直立环、速度环、转向环）
- 实时发送命令到小车
- 接收并绘制遥测数据曲线
- 支持 CSV 数据录制

使用方法：
python pid_tuner_gui.py
"""

import serial
import serial.tools.list_ports
import tkinter as tk
from tkinter import ttk, messagebox
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
import threading
import time
from collections import deque
from datetime import datetime
import csv

class PIDTunerGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("🔧 平衡车 PID 调参工具 v1.0")
        self.root.geometry("1400x800")
        
        # 串口配置
        self.ser = None
        self.com_port = ""
        self.baudrate = 115200
        self.is_connected = False
        
        # 数据存储（最大 200 个点）
        self.max_points = 200
        self.time_data = deque(maxlen=self.max_points)
        self.angle_data = deque(maxlen=self.max_points)
        self.gyro_data = deque(maxlen=self.max_points)
        self.pwm_data = deque(maxlen=self.max_points)
        
        # CSV 录制
        self.recording = False
        self.csv_file = None
        self.csv_writer = None
        
        # 创建 UI
        self.create_menu()
        self.create_widgets()
        
        # 启动数据接收线程
        self.rx_thread = threading.Thread(target=self.rx_loop, daemon=True)
        self.running = True
        self.rx_thread.start()
        
        # 自动刷新串口列表
        self.refresh_ports()
        
    def create_menu(self):
        """创建菜单栏"""
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        # 文件菜单
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="文件", menu=file_menu)
        file_menu.add_command(label="开始录制 CSV", command=self.start_recording)
        file_menu.add_command(label="停止录制", command=self.stop_recording)
        file_menu.add_separator()
        file_menu.add_command(label="退出", command=self.on_closing)
        
        # 帮助菜单
        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="帮助", menu=help_menu)
        help_menu.add_command(label="使用说明", command=self.show_help)
        help_menu.add_command(label="关于", command=self.show_about)
        
    def create_widgets(self):
        """创建主界面控件"""
        # 主分割框
        paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # === 左侧控制面板 ===
        left_frame = ttk.Frame(paned, width=350)
        paned.add(left_frame, weight=1)
        
        # 1. 串口配置区域
        serial_group = ttk.LabelFrame(left_frame, text="📡 串口配置", padding="10")
        serial_group.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Label(serial_group, text="COM 端口:").grid(column=0, row=0, sticky=tk.W, pady=2)
        self.com_combo = ttk.Combobox(serial_group, width=27, state="readonly")
        self.com_combo.grid(column=1, row=0, pady=2)
        
        ttk.Button(serial_group, text="🔄 刷新", command=self.refresh_ports).grid(column=2, row=0, padx=2, pady=2)
        
        self.connect_btn = ttk.Button(serial_group, text="🔌 连接", command=self.toggle_connection)
        self.connect_btn.grid(column=0, row=1, columnspan=3, pady=10, sticky="ew")
        
        # 连接状态指示灯
        self.status_label = ttk.Label(serial_group, text="❌ 未连接", foreground="red")
        self.status_label.grid(column=0, row=2, columnspan=3, pady=2)
        
        # 2. 直立环 PID 调节
        angle_group = ttk.LabelFrame(left_frame, text="⚖️ 直立环 PID (Angle)", padding="10")
        angle_group.pack(fill=tk.X, padx=5, pady=5)
        
        # Kp
        ttk.Label(angle_group, text="Kp:").grid(column=0, row=0, sticky=tk.W, pady=2)
        self.angle_kp_var = tk.DoubleVar(value=1.3)
        self.angle_kp_scale = ttk.Scale(angle_group, from_=0, to=10, variable=self.angle_kp_var, orient='horizontal')
        self.angle_kp_scale.grid(column=1, row=0, sticky="ew", pady=2)
        self.angle_kp_label = ttk.Label(angle_group, text="1.30", width=6)
        self.angle_kp_label.grid(column=2, row=0, pady=2)
        self.angle_kp_scale.configure(command=lambda v: self.angle_kp_label.config(text=f"{float(v):.2f}"))
        
        # Ki
        ttk.Label(angle_group, text="Ki:").grid(column=0, row=1, sticky=tk.W, pady=2)
        self.angle_ki_var = tk.DoubleVar(value=0.0)
        self.angle_ki_scale = ttk.Scale(angle_group, from_=-1, to=1, variable=self.angle_ki_var, orient='horizontal')
        self.angle_ki_scale.grid(column=1, row=1, sticky="ew", pady=2)
        self.angle_ki_label = ttk.Label(angle_group, text="0.00", width=6)
        self.angle_ki_label.grid(column=2, row=1, pady=2)
        self.angle_ki_scale.configure(command=lambda v: self.angle_ki_label.config(text=f"{float(v):.2f}"))
        
        # Kd
        ttk.Label(angle_group, text="Kd:").grid(column=0, row=2, sticky=tk.W, pady=2)
        self.angle_kd_var = tk.DoubleVar(value=1.5)
        self.angle_kd_scale = ttk.Scale(angle_group, from_=0, to=5, variable=self.angle_kd_var, orient='horizontal')
        self.angle_kd_scale.grid(column=1, row=2, sticky="ew", pady=2)
        self.angle_kd_label = ttk.Label(angle_group, text="1.50", width=6)
        self.angle_kd_label.grid(column=2, row=2, pady=2)
        self.angle_kd_scale.configure(command=lambda v: self.angle_kd_label.config(text=f"{float(v):.2f}"))
        
        ttk.Button(angle_group, text="应用参数", command=lambda: self.apply_params("angle")).grid(column=0, row=3, columnspan=3, pady=10, sticky="ew")
        
        # 3. 速度环 PID 调节
        speed_group = ttk.LabelFrame(left_frame, text="🚀 速度环 PID (Speed)", padding="10")
        speed_group.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Label(speed_group, text="Kp:").grid(column=0, row=0, sticky=tk.W, pady=2)
        self.speed_kp_var = tk.DoubleVar(value=0.3)
        ttk.Scale(speed_group, from_=0, to=2, variable=self.speed_kp_var, orient='horizontal').grid(column=1, row=0, sticky="ew", pady=2)
        ttk.Label(speed_group, text="0.30", width=6).grid(column=2, row=0, pady=2)
        
        ttk.Label(speed_group, text="Ki:").grid(column=0, row=1, sticky=tk.W, pady=2)
        self.speed_ki_var = tk.DoubleVar(value=0.05)
        ttk.Scale(speed_group, from_=0, to=0.5, variable=self.speed_ki_var, orient='horizontal').grid(column=1, row=1, sticky="ew", pady=2)
        ttk.Label(speed_group, text="0.05", width=6).grid(column=2, row=1, pady=2)
        
        ttk.Label(speed_group, text="Kd:").grid(column=0, row=2, sticky=tk.W, pady=2)
        self.speed_kd_var = tk.DoubleVar(value=0.0)
        ttk.Scale(speed_group, from_=0, to=1, variable=self.speed_kd_var, orient='horizontal').grid(column=1, row=2, sticky="ew", pady=2)
        ttk.Label(speed_group, text="0.00", width=6).grid(column=2, row=2, pady=2)
        
        ttk.Button(speed_group, text="应用参数", command=lambda: self.apply_params("speed")).grid(column=0, row=3, columnspan=3, pady=10, sticky="ew")
        
        # 4. 转向环 PID 调节
        turn_group = ttk.LabelFrame(left_frame, text="🔄 转向环 PID (Turn)", padding="10")
        turn_group.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Label(turn_group, text="Kp:").grid(column=0, row=0, sticky=tk.W, pady=2)
        self.turn_kp_var = tk.DoubleVar(value=0.8)
        ttk.Scale(turn_group, from_=0, to=5, variable=self.turn_kp_var, orient='horizontal').grid(column=1, row=0, sticky="ew", pady=2)
        ttk.Label(turn_group, text="0.80", width=6).grid(column=2, row=0, pady=2)
        
        ttk.Label(turn_group, text="Ki:").grid(column=0, row=1, sticky=tk.W, pady=2)
        self.turn_ki_var = tk.DoubleVar(value=0.0)
        ttk.Scale(turn_group, from_=0, to=0.5, variable=self.turn_ki_var, orient='horizontal').grid(column=1, row=1, sticky="ew", pady=2)
        ttk.Label(turn_group, text="0.00", width=6).grid(column=2, row=1, pady=2)
        
        ttk.Label(turn_group, text="Kd:").grid(column=0, row=2, sticky=tk.W, pady=2)
        self.turn_kd_var = tk.DoubleVar(value=0.2)
        ttk.Scale(turn_group, from_=0, to=2, variable=self.turn_kd_var, orient='horizontal').grid(column=1, row=2, sticky="ew", pady=2)
        ttk.Label(turn_group, text="0.20", width=6).grid(column=2, row=2, pady=2)
        
        ttk.Button(turn_group, text="应用参数", command=lambda: self.apply_params("turn")).grid(column=0, row=3, columnspan=3, pady=10, sticky="ew")
        
        # 5. 快捷操作
        quick_group = ttk.LabelFrame(left_frame, text="⚡ 快捷操作", padding="10")
        quick_group.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Button(quick_group, text="🔴 急停（所有 PWM=0）", command=self.emergency_stop).grid(column=0, row=0, columnspan=2, sticky="ew", pady=2)
        ttk.Button(quick_group, text="📊 重置图表", command=self.reset_plot).grid(column=0, row=1, columnspan=2, sticky="ew", pady=2)
        
        # === 右侧绘图区域 ===
        right_frame = ttk.Frame(paned)
        paned.add(right_frame, weight=3)
        
        # Matplotlib 图形
        self.fig, (self.ax1, self.ax2) = plt.subplots(2, 1, figsize=(10, 8))
        self.canvas = FigureCanvasTkAgg(self.fig, master=right_frame)
        self.canvas.draw()
        
        # 添加工具栏
        toolbar = NavigationToolbar2Tk(self.canvas, right_frame)
        toolbar.update()
        toolbar.pack(side=tk.TOP, fill=tk.X)
        
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        
        # 初始化图表
        self.init_plots()
        
    def init_plots(self):
        """初始化图表"""
        # 上图：角度、角速度、PWM
        self.ax1.clear()
        self.ax1.set_title('机械量 (Mechanical Variables)')
        self.ax1.set_xlabel('时间 (ms)')
        self.ax1.set_ylabel('数值')
        self.ax1.grid(True, linestyle='--', alpha=0.7)
        self.line_angle, = self.ax1.plot([], [], 'r-', label='Angle', linewidth=2)
        self.line_gyro, = self.ax1.plot([], [], 'g-', label='Gyro', linewidth=2)
        self.line_pwm, = self.ax1.plot([], [], 'b-', label='PWM', linewidth=2)
        self.ax1.legend(loc='upper right')
        
        # 下图：PID 参数
        self.ax2.clear()
        self.ax2.set_title('PID 参数 (PID Parameters)')
        self.ax2.set_xlabel('时间 (ms)')
        self.ax2.set_ylabel('增益值')
        self.ax2.grid(True, linestyle='--', alpha=0.7)
        self.line_kp, = self.ax2.plot([], [], 'm-', label='Kp', linewidth=2)
        self.line_ki, = self.ax2.plot([], [], 'c-', label='Ki', linewidth=2)
        self.line_kd, = self.ax2.plot([], [], 'y-', label='Kd', linewidth=2)
        self.ax2.legend(loc='upper right')
        
        self.fig.tight_layout()
        
    def refresh_plots(self):
        """刷新图表数据"""
        if len(self.time_data) == 0:
            return
            
        times = list(self.time_data)
        
        # 更新上图
        self.line_angle.set_data(times, list(self.angle_data))
        self.line_gyro.set_data(times, list(self.gyro_data))
        self.line_pwm.set_data(times, list(self.pwm_data))
        
        self.ax1.relim()
        self.ax1.autoscale_view()
        
        # 更新下图（显示当前 PID 参数）
        kp_val = [self.angle_kp_var.get()] * len(times)
        ki_val = [self.angle_ki_var.get()] * len(times)
        kd_val = [self.angle_kd_var.get()] * len(times)
        
        self.line_kp.set_data(times, kp_val)
        self.line_ki.set_data(times, ki_val)
        self.line_kd.set_data(times, kd_val)
        
        self.ax2.relim()
        self.ax2.autoscale_view()
        
        self.canvas.draw_idle()
        
    def refresh_ports(self):
        """刷新串口列表"""
        ports = serial.tools.list_ports.comports()
        port_list = [f"{port.device}" for port in ports]
        self.com_combo['values'] = port_list
        if port_list:
            self.com_combo.current(0)
            
    def toggle_connection(self):
        """切换串口连接状态"""
        if not self.is_connected:
            # 连接
            try:
                self.com_port = self.com_combo.get()
                if not self.com_port:
                    messagebox.showerror("错误", "请先选择 COM 端口")
                    return
                    
                self.ser = serial.Serial(self.com_port, self.baudrate, timeout=0.1)
                self.is_connected = True
                self.connect_btn.config(text="断开")
                self.status_label.config(text="✅ 已连接", foreground="green")
                print(f"已连接到 {self.com_port}")
            except Exception as e:
                messagebox.showerror("连接失败", f"无法打开串口:\n{e}")
        else:
            # 断开
            if self.ser and self.ser.is_open:
                self.ser.close()
            self.is_connected = False
            self.connect_btn.config(text="🔌 连接")
            self.status_label.config(text="❌ 未连接", foreground="red")
            print("已断开连接")
            
    def apply_params(self, target):
        """应用 PID 参数"""
        if not self.is_connected:
            messagebox.showwarning("警告", "未连接串口，无法发送参数")
            return
        
        if target == "angle":
            kp = self.angle_kp_var.get()
            ki = self.angle_ki_var.get()
            kd = self.angle_kd_var.get()
        elif target == "speed":
            kp = self.speed_kp_var.get()
            ki = self.speed_ki_var.get()
            kd = self.speed_kd_var.get()
        elif target == "turn":
            kp = self.turn_kp_var.get()
            ki = self.turn_ki_var.get()
            kd = self.turn_kd_var.get()
        
        cmd = f"[{target},{kp},{ki},{kd}]\n"
        self.ser.write(cmd.encode())
        print(f"发送命令：{cmd.strip()}")
        
    def emergency_stop(self):
        """急停：将所有 PID 输出设为 0"""
        if not self.is_connected:
            return
            
        # 发送特殊命令（需要在下位机实现）
        cmd = "[stop,0,0,0]\n"
        self.ser.write(cmd.encode())
        messagebox.showinfo("急停", "已发送急停命令\n请立即检查小车状态！")
        
    def reset_plot(self):
        """重置图表"""
        self.time_data.clear()
        self.angle_data.clear()
        self.gyro_data.clear()
        self.pwm_data.clear()
        self.init_plots()
        
    def rx_loop(self):
        """数据接收循环（后台线程）"""
        while self.running:
            if self.is_connected and self.ser and self.ser.is_open:
                try:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    
                    if line and not line.startswith('#') and not line.startswith('[OK]'):
                        # 解析 CSV 数据：Time_ms,Angle,Gyro,PWM,...
                        parts = line.split(',')
                        if len(parts) >= 4:
                            try:
                                timestamp = float(parts[0])
                                angle = float(parts[1])
                                gyro = float(parts[2])
                                pwm = float(parts[3])
                                
                                self.time_data.append(timestamp)
                                self.angle_data.append(angle)
                                self.gyro_data.append(gyro)
                                self.pwm_data.append(pwm)
                                
                                # 更新 GUI
                                self.root.after(0, self.refresh_plots)
                                
                                # CSV 录制
                                if self.recording and self.csv_writer:
                                    self.csv_writer.writerow([timestamp, angle, gyro, pwm])
                                    
                            except ValueError:
                                pass  # 忽略格式错误的数据
                                
                except Exception as e:
                    pass  # 忽略解码错误
                    
            time.sleep(0.01)
            
    def start_recording(self):
        """开始 CSV 录制"""
        if self.recording:
            return
            
        filename = f"pid_data_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        self.csv_file = open(filename, 'w', newline='')
        self.csv_writer = csv.writer(self.csv_file)
        self.csv_writer.writerow(['Timestamp_ms', 'Angle', 'Gyro', 'PWM'])
        self.recording = True
        print(f"开始录制：{filename}")
        
    def stop_recording(self):
        """停止 CSV 录制"""
        if not self.recording:
            return
            
        self.recording = False
        if self.csv_file:
            self.csv_file.close()
            self.csv_file = None
        print("录制完成")
        
    def show_help(self):
        """显示帮助"""
        help_text = """
        🔧 平衡车 PID 调参工具使用说明
        
        1. 连接串口:
           - 选择正确的 COM 端口
           - 点击"连接"按钮
           
        2. 调节 PID 参数:
           - 拖动滑动条调整 Kp/Ki/Kd
           - 点击"应用参数"发送到小车
           
        3. 观察响应:
           - 右侧图表实时显示角度、角速度、PWM
           - 根据响应曲线优化参数
           
        4. 数据录制:
           - 文件 → 开始录制 CSV
           - 保存测试数据用于后续分析
           
        通信协议:
        [angle,Kp,Ki,Kd] - 设置直立环参数
        [speed,Kp,Ki,Kd] - 设置速度环参数
        [turn,Kp,Ki,Kd]  - 设置转向环参数
        """
        messagebox.showinfo("使用说明", help_text)
        
    def show_about(self):
        """显示关于"""
        about_text = """
        🔧 平衡车 PID 调参工具 v1.0
        
        作者：Eureka
        日期：2026-03-20
        
        基于 STM32F103C8T6 平衡车项目
        使用 PlatformIO 开发环境
        
        GitHub: [项目地址]
        """
        messagebox.showinfo("关于", about_text)
        
    def on_closing(self):
        """窗口关闭处理"""
        self.running = False
        if self.ser and self.ser.is_open:
            self.ser.close()
        if self.csv_file:
            self.csv_file.close()
        self.root.destroy()

if __name__ == '__main__':
    root = tk.Tk()
    app = PIDTunerGUI(root)
    root.mainloop()
