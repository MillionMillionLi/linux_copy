#!/bin/bash

# sudo apt install sshpass  wget # 需要安装以上两个程序

commit_id="dfaf44141ea9deb3b4096f7cd6d24e00c147a4b1" # 你的vscode版本 菜单栏->帮助->关于
host="139.129.24.65" #替换为你需要安装的PC host名
user="root" # 在远程host中你的账号
password="111111Lml" # 将xxx替换为实际密码
pkg_name="stable.1"
host_dir=".vscode-server/bin"

# 修复这里的变量引用错误
sshpass -p "${password}" ssh "${user}@${host}" "mkdir -p ~/.vscode-server;mkdir -p ~/${host_dir}"

# 将文件复制到远程服务器
sshpass -p "${password}" scp "${pkg_name}" "${user}@${host}:~/${host_dir}/"

# 解压文件并将其移到正确的目录
sshpass -p "${password}" ssh "${user}@${host}" "tar -xvf ~/${host_dir}/${pkg_name} -C ~/${host_dir}; mv ~/${host_dir}/vscode-server-linux-x64  ~/${host_dir}/${commit_id}"

# 同步本地扩展到远程服务器
sshpass -p "${password}" rsync -avz --progress ~/.vscode/extensions/ "${user}@${host}:~/.vscode-server/extensions/"
