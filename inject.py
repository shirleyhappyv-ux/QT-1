import sys
import os
import time

def inject_text():
    if len(sys.argv) < 2:
        print("提示: 请输入要注入的中文内容，例如: python3 inject.py 北京市")
        return
        
    text = sys.argv[1]
    
    print( "请在 3 秒内切换到 noVNC 窗口，并用鼠标点击激活 Qt 的搜索输入框..." )
    for i in range(3, 0, -1):
        print(f"{i}...")
        time.sleep(1)
        
    print(f"🚀 开始向输入框强行注入: {text}")
    
    # 将中文复制到虚拟系统内部的真实剪贴板中 (绕过 noVNC 通道)
    os.system(f'echo "{text}" | DISPLAY=:1 xclip -selection clipboard 2>/dev/null || echo "{text}" > /tmp/vnc_text.txt')
    
    # 终极一招：利用 xdotool 直接在屏幕 :1 上模拟真实的键盘打字，强行吐出中文字符
    os.system(f'DISPLAY=:1 xdotool type "{text}"')
    print("✅ 注入完成！检查你的 Qt 输入框！")

if __name__ == "__main__":
    inject_text()
