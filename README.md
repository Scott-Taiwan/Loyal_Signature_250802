# 忠誠度簽名程式

此程式用於對CSV文件進行簽名驗證，確保數據完整性和真實性。

## 依賴項

- OpenSSL函式庫（用於加密和簽名）
- C++11相容的編譯器

## 編譯方法

在Linux環境下，使用以下命令編譯程式：

```bash
make
```

## 使用方法

### 生成RSA密鑰對

```bash
./loyalty_signer genkey <私鑰輸出文件> <公鑰輸出文件>
```

例如：
```bash
./loyalty_signer genkey private_key.pem public_key.pem
```

### 對CSV文件進行簽名

```bash
./loyalty_signer sign <輸入CSV文件> <輸出CSV文件> <私鑰文件>
```

例如：
```bash
./loyalty_signer sign input.csv output.csv private_key.pem
```

### 驗證CSV文件簽名

```bash
./loyalty_signer verify <CSV文件> <公鑰文件>
```

例如：
```bash
./loyalty_signer verify output.csv public_key.pem
```
