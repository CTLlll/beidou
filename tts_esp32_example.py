from machine import UART, Pin
import time
class VTX316:
    def __init__(self, tx_pin: int, rx_pin: int, baudrate=115200, uart_id=2):
        """初始化 VTX316 语音模块"""
        self.uart = UART(uart_id, baudrate=baudrate, tx=Pin(tx_pin), rx=Pin(rx_pin))
        time.sleep_ms(200)

    def _send_bytes(self, data: bytes):
        """发送原始字节"""
        self.uart.write(data)

    def _send_text(self, text: str):
        """发送字符串文本"""
        self.uart.write(text.encode('gbk'))

    def bobo(self, message: str):
        if not message:
            return
        msg_bytes = message.encode('gbk')
        length = len(msg_bytes)
        frame_head = bytes([0xFD, 0x00, length + 2, 0x01, 0x05])
        self._send_bytes(frame_head)
        time.sleep_ms(2)
        self._send_bytes(msg_bytes)
        time.sleep_ms(length + 50)

    def yinliang(self, level: int):
        if level < 0:
            level = 0
        elif level > 10:
            level = 10
        frame = bytes([
            0xFD, 0x00, 0x06,
            0x01, 0x01,
            0x5B, 0x76,
            0x30 + level,
            0x5D
        ])
        self._send_bytes(frame)
        time.sleep_ms(100)

    def zanting(self):
        cmd = bytes([0xFD, 0x00, 0x01, 0x03])
        self._send_bytes(cmd)

    def huifu(self):
        cmd = bytes([0xFD, 0x00, 0x01, 0x04])
        self._send_bytes(cmd)

    def fayanren(self, idx: int):
        if idx < 1 or idx > 7:
            idx = 1
        cmd_str = f"[m5{idx}]"
        msg_bytes = cmd_str.encode('gbk')
        length = len(msg_bytes)

        frame_head = bytes([0xFD, 0x00, length + 2, 0x01, 0x05])
        self._send_bytes(frame_head)
        time.sleep_ms(2)
        self._send_bytes(msg_bytes)
        time.sleep_ms(50)



voice = VTX316(tx_pin=17, rx_pin=16)

voice.yinliang(6)

voice.bobo('欢迎使用语音合成模块')