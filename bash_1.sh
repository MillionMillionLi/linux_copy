#!/bin/bash

# sudo apt install sshpass  wget # 需要本地安装以上两个程序

commit_id="dfaf44141ea9deb3b4096f7cd6d24e00c147a4b1" # 你的vscode版本 菜单栏->帮助->关于
host="139.129.24.65" #替换为你需要安装的PC host名
user="aliyun8433653310" # 在远程host中你的账号
password="111111Lml" # 将xxx替换为实际密码

pkg_name="stable"
host_dir=".vscode-server/bin"

# # 下载 Visual Studio Code 服务器

wget "https://update.code.visualstudio.com/commit:${commit_id}/server-linux-x64/${pkg_name}"
#wget "https://update.code.visualstudio.com/commit:abd2f3db4bdb28f9e95536dfa84d8479f1eb312d/server-linux-x64/stable"
