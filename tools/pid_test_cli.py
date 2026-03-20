#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PID 参数快速测试脚本

功能：
- 命令行交互式调参
- 自动扫描 Kp 值寻找临界增益
- 批量测试多组参数

使用方法：
python pid_test_cli.py
"""

import serial
import serial.tools.list_ports
import time
import sys

class PIDTestCLI:
    def __init__(self):
        self.ser = None
        self.com_port = ""
        self.baudrate = 115200
        
    def list_ports(self):
        """列出可用串口"""
        ports = serial.tools.list_ports.comports()
        print("\n📡 可用 COM 端口:")
        for i, port in enumerate(ports):
            print(f"  {i+1}. {port.device} - {port.description}")
        return ports
        
    def connect(self, port_name):
        """连接串口"""
        try:
            self.ser = serial.Serial(port_name, self.baudrate, timeout=0.5)
            time.sleep(1)  # 等待串口稳定
            print(f"✅ 已连接到 {port_name}")
            return True
        except Exception as e:
            print(f"❌ 连接失败：{e}")
            return False
            
    def disconnect(self):
        """断开连接"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("🔌 已断开连接")
            
    def send_command(self, target, kp, ki, kd):
        """发送 PID 参数命令"""
        cmd = f"[{target},{kp},{ki},{kd}]\r\n"
        self.ser.write(cmd.encode())
        print(f"📤 发送：{cmd.strip()}")
        
        # 等待响应
        time.sleep(0.1)
        response = self.ser.readline().decode('utf-8', errors='ignore').strip()
        if response:
            print(f"📥 响应：{response}")
            
    def set_angle_pid(self, kp, ki, kd):
        """设置直立环 PID"""
        self.send_command("angle", kp, ki, kd)
        
    def set_speed_pid(self, kp, ki, kd):
        """设置速度环 PID"""
        self.send_command("speed", kp, ki, kd)
        
    def set_turn_pid(self, kp, ki, kd):
        """设置转向环 PID"""
        self.send_command("turn", kp, ki, kd)
        
    def monitor_data(self, duration=10):
        """监控遥测数据"""
        print(f"\n📊 开始监控 {duration} 秒...")
        print("按 Ctrl+C 停止\n")
        
        start_time = time.time()
        count = 0
        
        try:
            while time.time() - start_time < duration:
                if self.ser.in_waiting > 0:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    
                    # 过滤掉命令包和表头
                    if line and not line.startswith('#') and not line.startswith('[OK]'):
                        parts = line.split(',')
                        if len(parts) >= 4:
                            try:
                                timestamp = float(parts[0])
                                angle = float(parts[1])
                                gyro = float(parts[2])
                                pwm = float(parts[3])
                                
                                # 简单统计
                                count += 1
                                if count % 10 == 0:  # 每 10 条显示一次
                                    print(f"t={timestamp:6.1f}ms | Angle={angle:7.3f}° | Gyro={gyro:7.2f}°/s | PWM={pwm:7.1f}")
                                    
                            except ValueError:
                                pass
                                
                time.sleep(0.01)
                
        except KeyboardInterrupt:
            pass
            
        print(f"\n📈 共接收 {count} 条数据")
        
    def auto_find_critical_kp(self):
        """自动寻找临界 Kp"""
        print("\n🔍 开始自动寻找临界 Kp...")
        print("⚠️ 请确保小车已固定，电机悬空！\n")
        
        kp = 0.5
        step = 0.5
        
        print("逐步增加 Kp，观察是否出现等幅振荡...")
        print("当看到持续振荡时，输入 'y' 确认，否则按 Enter 继续\n")
        
        while kp <= 10:
            self.set_angle_pid(kp, 0, 0)
            print(f"\n当前 Kp = {kp:.1f}")
            
            # 等待响应
            time.sleep(1)
            
            # 读取几条数据
            for _ in range(5):
                if self.ser.in_waiting > 0:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line and not line.startswith('#'):
                        print(f"  {line}")
                        
            response = input(f"Kp={kp:.1f} 是否出现等幅振荡？(y/n): ").strip().lower()
            if response == 'y':
                print(f"\n✅ 找到临界 Kp ≈ {kp:.1f}")
                print(f"建议初始参数:")
                print(f"  Kp = {kp * 0.6:.2f}")
                print(f"  Ki = {(2 * kp * 0.6) / 2.0:.2f} (假设 Tu=2s)")
                print(f"  Kd = {(kp * 0.6 * 2.0) / 8:.2f}")
                return kp
                
            kp += step
            
        print("\n❌ 未找到临界点（Kp 已超过 10）")
        return None
        
    def interactive_mode(self):
        """交互模式"""
        print("\n" + "="*50)
        print("🔧 PID 参数调试交互模式")
        print("="*50)
        print("\n命令列表:")
        print("  a <kp> <ki> <kd>  - 设置直立环参数")
        print("  s <kp> <ki> <kd>  - 设置速度环参数")
        print("  t <kp> <ki> <kd>  - 设置转向环参数")
        print("  m [seconds]       - 监控数据（默认 10 秒）")
        print("  auto              - 自动寻找临界 Kp")
        print("  q                 - 退出")
        print("="*50 + "\n")
        
        while True:
            try:
                cmd = input(">>> ").strip().split()
                if not cmd:
                    continue
                    
                if cmd[0] == 'q':
                    break
                elif cmd[0] == 'a' and len(cmd) >= 4:
                    kp, ki, kd = float(cmd[1]), float(cmd[2]), float(cmd[3])
                    self.set_angle_pid(kp, ki, kd)
                elif cmd[0] == 's' and len(cmd) >= 4:
                    kp, ki, kd = float(cmd[1]), float(cmd[2]), float(cmd[3])
                    self.set_speed_pid(kp, ki, kd)
                elif cmd[0] == 't' and len(cmd) >= 4:
                    kp, ki, kd = float(cmd[1]), float(cmd[2]), float(cmd[3])
                    self.set_turn_pid(kp, ki, kd)
                elif cmd[0] == 'm':
                    duration = int(cmd[1]) if len(cmd) > 1 else 10
                    self.monitor_data(duration)
                elif cmd[0] == 'auto':
                    self.auto_find_critical_kp()
                else:
                    print("❌ 未知命令或参数不足")
                    
            except ValueError:
                print("❌ 参数格式错误，请输入数字")
            except KeyboardInterrupt:
                print("\n⚠️ 中断操作")
                break
                
    def run(self):
        """主程序入口"""
        print("\n" + "="*50)
        print("🔧 平衡车 PID 参数测试工具")
        print("="*50)
        
        # 选择串口
        ports = self.list_ports()
        if not ports:
            print("❌ 未找到可用串口")
            return
            
        try:
            choice = int(input(f"\n请选择 COM 端口 (1-{len(ports)}): "))
            if 1 <= choice <= len(ports):
                self.com_port = ports[choice-1].device
            else:
                print("❌ 无效选择")
                return
        except ValueError:
            print("❌ 无效输入")
            return
            
        # 连接
        if not self.connect(self.com_port):
            return
            
        # 进入交互模式
        self.interactive_mode()
        
        # 断开
        self.disconnect()
        print("\n👋 再见！")

if __name__ == '__main__':
    cli = PIDTestCLI()
    cli.run()
