import selectors
import socket
import os
import json
import time
import sys
import uuid
import asyncio
import threading
import queue
from datetime import datetime
import csv
import os


async def mt_func_with_json_filename(dataFile, isPrintRecvResponse=True):
    # 创建一个客户端的socket对象
    reader, writer = await asyncio.open_connection("127.0.0.1", 10004)

    # 获取当前工作目录
    current_dir = os.getcwd()

    # 构建文件的绝对路径
    file_path = os.path.join(current_dir, dataFile)

    data = None  # 初始化 content 变量
    # 从 JSON 文件中读取数据
    with open(file_path, 'r') as file:
        data = json.load(file)

    data2 = "W{}QUIT".format(json.dumps(data))
    send_data2 = data2.encode(encoding="utf8")
    writer.write(send_data2)

    await writer.drain()

    msg = await reader.read(819600)
    # 接收服务端返回的数据，需要解码
    if isPrintRecvResponse:
        print(msg.decode("utf8"))
    
    print("{} done!".format(funcName))

    #记得关闭资源
    writer.close()  # 关闭连接
    await writer.wait_closed()  # 等待连接关闭

async def main():
    jsonFileName = 'select_account.json' 
    await mt_func_with_json_filename(jsonFileName)

asyncio.run(main())




