#!/bin/bash

# 編譯程式
echo "正在編譯程式..."
make

if [ $? -ne 0 ]; then
    echo "編譯失敗！"
    exit 1
fi

echo "編譯成功！"

# 生成RSA密鑰對
echo "正在生成RSA密鑰對..."
./loyalty_signer genkey private_key.pem public_key.pem

if [ $? -ne 0 ]; then
    echo "生成RSA密鑰對失敗！"
    exit 1
fi

# 對CSV文件進行簽名
echo "正在對CSV文件進行簽名..."
./loyalty_signer sign FPD_22_20250604_悠遊卡公司_before.csv FPD_22_20250604_悠遊卡公司_final.csv private_key.pem

if [ $? -ne 0 ]; then
    echo "簽名失敗！"
    exit 1
fi

echo "簽名成功！文件已保存為 FPD_22_20250604_悠遊卡公司_final.csv"

# 驗證簽名
echo "正在驗證簽名..."
./loyalty_signer verify FPD_22_20250604_悠遊卡公司_final.csv public_key.pem

if [ $? -ne 0 ]; then
    echo "驗證失敗！"
    exit 1
fi

echo "驗證成功！"
echo "處理完成！"
