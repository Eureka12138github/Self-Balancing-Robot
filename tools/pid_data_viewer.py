#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
平衡车 PID 调试数据可视化工具
功能：
1. 实时接收串口数据
2. 绘制角度、角速度、PWM 波形
3. 自动记录数据到 CSV
4. 支持离线数据分析

使用方法：
python pid_data_viewer.py COM3 115200
"""

import serial
import sys
import time
import csv
from datetime import datetime
from collections import deque

# 尝试导入 matplotlib
try:
    import matplotlib.pyplot as plt
    import matplotlib.animation as animation
    MATPLOTLIB_AVAILABLE = True
except ImportError:
    print("❌ 未找到 matplotlib，请先安装：pip install matplotlib")
    MATPLOTLIB_AVAILABLE = False

# ============ 配置参数 ============
class Config:
    # 串口配置
    SERIAL_PORT = 'COM3'  # 根据实际情况修改
    BAUDRATE = 115200
    TIMEOUT = 0.1
    
    # 数据显示
    MAX_POINTS = 200  # 显示最近 200 个数据点
    
    # 数据列定义
    # 格式：timestamp,angle,gyro,pwm_out,Kp,Ki,Kd
    COL_TIMESTAMP = 0
    COL_ANGLE = 1
    COL_GYRO = 2
    COL_PWM = 3
    COL_KP = 4
    COL_KI = 5
    COL_KD = 6

# ============ 数据容器 ============
class DataBuffer:
    def __init__(self, max_points=200):
        self.max_points = max_points
        self.timestamps = deque(maxlen=max_points)
        self.angles = deque(maxlen=max_points)
        self.gyros = deque(maxlen=max_points)
        self.pwms = deque(maxlen=max_points)
        self.kps = deque(maxlen=max_points)
        self.kis = deque(maxlen=max_points)
        self.kds = deque(maxlen=max_points)
        
        # 统计数据
        self.total_lines = 0
        self.error_lines = 0
        
    def append(self, data):
        """添加一行数据"""
        try:
            self.timestamps.append(data[Config.COL_TIMESTAMP])
            self.angles.append(data[Config.COL_ANGLE])
            self.gyros.append(data[Config.COL_GYRO])
            self.pwms.append(data[Config.COL_PWM])
            self.kps.append(data[Config.COL_KP])
            self.kis.append(data[Config.COL_KI])
            self.kds.append(data[Config.COL_KD])
            self.total_lines += 1
        except Exception as e:
            self.error_lines += 1
            print(f"⚠️ 数据解析错误：{e}")
    
    def clear(self):
        """清空缓冲区"""
        self.timestamps.clear()
        self.angles.clear()
        self.gyros.clear()
        self.pwms.clear()
        self.kps.clear()
        self.kis.clear()
        self.kds.clear()
        self.total_lines = 0
        self.error_lines = 0
    
    def get_lists(self):
        """获取列表形式的数据"""
        return (list(self.timestamps), list(self.angles), 
                list(self.gyros), list(self.pwms),
                list(self.kps), list(self.kis), list(self.kds))

# ============ 串口读取器 ============
class SerialReader:
    def __init__(self, port, baudrate, timeout=0.1):
        self.port = port
        self.baudrate = baudrate
        self.serial = None
        self.buffer = ""
        
    def connect(self):
        """连接串口"""
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=self.timeout
            )
            print(f"✅ 串口 {self.port} 打开成功")
            return True
        except Exception as e:
            print(f"❌ 串口打开失败：{e}")
            return False
    
    def disconnect(self):
        """断开串口"""
        if self.serial and self.serial.is_open:
            self.serial.close()
            print("📴 串口已关闭")
    
    def read_line(self):
        """读取一行数据"""
        try:
            if self.serial and self.serial.is_open:
                line = self.serial.readline().decode('utf-8', errors='ignore').strip()
                return line
        except Exception as e:
            print(f"⚠️ 读取错误：{e}")
        return None
    
    def parse_line(self, line):
        """解析一行数据"""
        try:
            # 过滤非数据行
            if not line or line.startswith('['):
                return None
            
            # 分割 CSV 数据
            parts = line.split(',')
            if len(parts) < 7:
                return None
            
            # 转换为浮点数
            data = [float(x) for x in parts]
            return data
            
        except Exception as e:
            return None

# ============ 数据保存 ============
def save_to_csv(filename, data_buffer):
    """保存数据到 CSV 文件"""
    try:
        with open(filename, 'w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            # 写入表头
            writer.writerow(['Timestamp(ms)', 'Angle(deg)', 'Gyro', 'PWM', 'Kp', 'Ki', 'Kd'])
            # 写入数据
            for i in range(len(data_buffer.timestamps)):
                writer.writerow([
                    data_buffer.timestamps[i],
                    data_buffer.angles[i],
                    data_buffer.gyros[i],
                    data_buffer.pwms[i],
                    data_buffer.kps[i],
                    data_buffer.kis[i],
                    data_buffer.kds[i]
                ])
        print(f"💾 数据已保存到：{filename}")
    except Exception as e:
        print(f"❌ 保存失败：{e}")

# ============ 可视化 ============
class PIDVisualizer:
    def __init__(self, data_buffer):
        self.data_buffer = data_buffer
        self.fig = None
        self.axs = None
        self.lines = None
        self ani = None
        
    def setup(self):
        """设置图形界面"""
        if not MATPLOTLIB_AVAILABLE:
            return False
        
        # 创建 3 个子图
        self.fig, self.axs = plt.subplots(3, 1, figsize=(12, 8))
        self.fig.suptitle('平衡车 PID 调试数据', fontsize=16)
        
        # 初始化线条
        self.lines = []
        
        # 子图 1: 角度
        self.axs[0].set_title('角度 (Angle)')
        self.axs[0].set_ylabel('度 (°)')
        self.axs[0].grid(True, alpha=0.3)
        line, = self.axs[0].plot([], [], 'b-', linewidth=1)
        self.lines.append(line)
        
        # 子图 2: 角速度
        self.axs[1].set_title('角速度 (Gyro)')
        self.axs[1].set_ylabel('度/秒')
        self.axs[1].grid(True, alpha=0.3)
        line, = self.axs[1].plot([], [], 'g-', linewidth=1)
        self.lines.append(line)
        
        # 子图 3: PWM 输出
        self.axs[2].set_title('PWM 输出')
        self.axs[2].set_ylabel('PWM')
        self.axs[2].set_xlabel('时间 (ms)')
        self.axs[2].grid(True, alpha=0.3)
        line, = self.axs[2].plot([], [], 'r-', linewidth=1)
        self.lines.append(line)
        
        # 调整布局
        plt.tight_layout()
        return True
    
    def update(self, frame):
        """更新图形数据"""
        timestamps, angles, gyros, pwms, kps, kis, kds = self.data_buffer.get_lists()
        
        if len(timestamps) == 0:
            return self.lines
        
        # 更新角度曲线
        self.lines[0].set_data(timestamps, angles)
        self.axs[0].relim()
        self.axs[0].autoscale_view()
        
        # 更新角速度曲线
        self.lines[1].set_data(timestamps, gyros)
        self.axs[1].relim()
        self.axs[1].autoscale_view()
        
        # 更新 PWM 曲线
        self.lines[2].set_data(timestamps, pwms)
        self.axs[2].relim()
        self.axs[2].autoscale_view()
        
        return self.lines
    
    def start(self):
        """启动动画"""
        if not MATPLOTLIB_AVAILABLE:
            return
        
        self.ani = animation.FuncAnimation(
            self.fig, self.update, interval=100, blit=True
        )
        plt.show()

# ============ 主程序 ============
def main():
    # 检查命令行参数
    if len(sys.argv) < 2:
        print("用法：python pid_data_viewer.py <COM 端口> [波特率]")
        print("示例：python pid_data_viewer.py COM3 115200")
        sys.exit(1)
    
    # 解析参数
    port = sys.argv[1]
    baudrate = int(sys.argv[2]) if len(sys.argv) > 2 else Config.BAUDRATE
    
    # 创建对象
    data_buffer = DataBuffer(max_points=Config.MAX_POINTS)
    serial_reader = SerialReader(port, baudrate)
    visualizer = PIDVisualizer(data_buffer)
    
    # 连接串口
    if not serial_reader.connect():
        sys.exit(1)
    
    # 等待设备就绪
    print("⏳ 等待设备发送数据...")
    time.sleep(2)
    
    # 清空缓冲区（跳过旧数据）
    while serial_reader.serial.in_waiting > 0:
        serial_reader.read_line()
    
    print(" 开始接收数据...")
    print("按 Ctrl+C 停止")
    
    # 保存文件名
    save_filename = f"pid_data_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
    
    try:
        # 设置可视化
        if visualizer.setup():
            visualizer.start()
        else:
            # 如果没有 matplotlib，只接收数据
            while True:
                line = serial_reader.read_line()
                if line:
                    data = serial_reader.parse_line(line)
                    if data:
                        data_buffer.append(data)
                        # 每 10 秒保存一次
                        if data_buffer.total_lines % 1000 == 0:
                            save_to_csv(save_filename, data_buffer)
                            print(f"📈 已接收 {data_buffer.total_lines} 条数据")
                
                time.sleep(0.01)
    
    except KeyboardInterrupt:
        print("\n\n🛑 停止接收")
    finally:
        # 保存数据
        if data_buffer.total_lines > 0:
            save_to_csv(save_filename, data_buffer)
            print(f"\n📊 数据统计:")
            print(f"   总数据：{data_buffer.total_lines} 条")
            print(f"   错误行：{data_buffer.error_lines} 条")
        
        # 断开串口
        serial_reader.disconnect()

if __name__ == '__main__':
    main()
