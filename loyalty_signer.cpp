#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <sstream>
#include <iomanip>

// 讀取CSV文件內容
std::vector<std::string> readCSV(const std::string& filename) {
    std::vector<std::string> lines;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "無法開啟文件: " << filename << std::endl;
        return lines;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Remove Windows line ending if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    file.close();
    return lines;
}

// 計算文件內容的SHA-256哈希
std::string calculateHash(const std::vector<std::string>& lines) {
    std::string content;
    // 連接除最後一行之外的所有行
    for (size_t i = 0; i < lines.size(); ++i) {
        content += lines[i];
        if (i < lines.size() - 1) {
            content += "\n";
        }
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, content.c_str(), content.length());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// 使用私鑰對數據進行簽名
std::string signData(const std::string& data, const std::string& privateKeyPath) {
    std::string signature;
    
    BIO* bioPrivKey = BIO_new_file(privateKeyPath.c_str(), "r");
    if (!bioPrivKey) {
        std::cerr << "無法開啟私鑰文件: " << privateKeyPath << std::endl;
        return signature;
    }

    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bioPrivKey, NULL, NULL, NULL);
    BIO_free(bioPrivKey);
    
    if (!pkey) {
        std::cerr << "無法讀取私鑰" << std::endl;
        ERR_print_errors_fp(stderr);
        return signature;
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (EVP_DigestSignInit(ctx, NULL, EVP_sha256(), NULL, pkey) <= 0) {
        std::cerr << "簽名初始化失敗" << std::endl;
        ERR_print_errors_fp(stderr);
        EVP_PKEY_free(pkey);
        EVP_MD_CTX_free(ctx);
        return signature;
    }

    if (EVP_DigestSignUpdate(ctx, data.c_str(), data.length()) <= 0) {
        std::cerr << "簽名更新失敗" << std::endl;
        ERR_print_errors_fp(stderr);
        EVP_PKEY_free(pkey);
        EVP_MD_CTX_free(ctx);
        return signature;
    }

    size_t siglen;
    if (EVP_DigestSignFinal(ctx, NULL, &siglen) <= 0) {
        std::cerr << "簽名長度獲取失敗" << std::endl;
        ERR_print_errors_fp(stderr);
        EVP_PKEY_free(pkey);
        EVP_MD_CTX_free(ctx);
        return signature;
    }

    unsigned char* sig = (unsigned char*)OPENSSL_malloc(siglen);
    if (!sig) {
        std::cerr << "記憶體分配失敗" << std::endl;
        EVP_PKEY_free(pkey);
        EVP_MD_CTX_free(ctx);
        return signature;
    }

    if (EVP_DigestSignFinal(ctx, sig, &siglen) <= 0) {
        std::cerr << "簽名最終生成失敗" << std::endl;
        ERR_print_errors_fp(stderr);
        OPENSSL_free(sig);
        EVP_PKEY_free(pkey);
        EVP_MD_CTX_free(ctx);
        return signature;
    }

    std::stringstream ss;
    for (size_t i = 0; i < siglen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)sig[i];
    }
    signature = ss.str();

    OPENSSL_free(sig);
    EVP_PKEY_free(pkey);
    EVP_MD_CTX_free(ctx);
    
    return signature;
}

// 驗證簽名
bool verifySignature(const std::string& data, const std::string& signature, const std::string& publicKeyPath) {
    BIO* bioPublicKey = BIO_new_file(publicKeyPath.c_str(), "r");
    if (!bioPublicKey) {
        std::cerr << "無法開啟公鑰文件: " << publicKeyPath << std::endl;
        return false;
    }

    EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bioPublicKey, NULL, NULL, NULL);
    BIO_free(bioPublicKey);
    
    if (!pkey) {
        std::cerr << "無法讀取公鑰" << std::endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // 將十六進制簽名轉換回二進制
    std::vector<unsigned char> binarySignature;
    for (size_t i = 0; i < signature.length(); i += 2) {
        std::string byteString = signature.substr(i, 2);
        unsigned char byte = (unsigned char)strtol(byteString.c_str(), NULL, 16);
        binarySignature.push_back(byte);
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (EVP_DigestVerifyInit(ctx, NULL, EVP_sha256(), NULL, pkey) <= 0) {
        std::cerr << "驗證初始化失敗" << std::endl;
        ERR_print_errors_fp(stderr);
        EVP_PKEY_free(pkey);
        EVP_MD_CTX_free(ctx);
        return false;
    }

    if (EVP_DigestVerifyUpdate(ctx, data.c_str(), data.length()) <= 0) {
        std::cerr << "驗證更新失敗" << std::endl;
        ERR_print_errors_fp(stderr);
        EVP_PKEY_free(pkey);
        EVP_MD_CTX_free(ctx);
        return false;
    }

    int result = EVP_DigestVerifyFinal(ctx, binarySignature.data(), binarySignature.size());

    EVP_PKEY_free(pkey);
    EVP_MD_CTX_free(ctx);

    return result > 0;
}

// 將簽名追加到CSV文件中
bool appendSignatureToCSV(const std::string& inputFilename, const std::string& outputFilename, const std::string& signature) {
    std::vector<std::string> lines = readCSV(inputFilename);
    if (lines.empty()) {
        return false;
    }

    std::ofstream outputFile(outputFilename);
    if (!outputFile.is_open()) {
        std::cerr << "無法創建輸出文件: " << outputFilename << std::endl;
        return false;
    }

    // 寫入原始內容
    for (const auto& line : lines) {
        outputFile << line << std::endl;
    }

    // 追加簽名行
    outputFile << "SIGNATURE|" << signature << std::endl;

    outputFile.close();
    return true;
}

void printUsage(const char* programName) {
    std::cout << "用法: " << programName << " <命令> [參數]" << std::endl;
    std::cout << "命令:" << std::endl;
    std::cout << "  sign <輸入CSV文件> <輸出CSV文件> <私鑰文件>" << std::endl;
    std::cout << "      - 使用私鑰對CSV文件內容進行簽名並將結果寫入輸出文件" << std::endl;
    std::cout << "  verify <CSV文件> <公鑰文件>" << std::endl;
    std::cout << "      - 驗證CSV文件的簽名是否有效" << std::endl;
    std::cout << "  genkey <私鑰輸出文件> <公鑰輸出文件>" << std::endl;
    std::cout << "      - 生成RSA密鑰對" << std::endl;
}

// 生成RSA密鑰對
bool generateRSAKeyPair(const std::string& privateKeyFile, const std::string& publicKeyFile) {
    int bits = 2048;
    
    // 初始化OpenSSL
    OpenSSL_add_all_algorithms();
    
    // 創建RSA結構
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (!ctx) {
        std::cerr << "創建PKEY上下文失敗" << std::endl;
        return false;
    }
    
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        std::cerr << "密鑰生成初始化失敗" << std::endl;
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits) <= 0) {
        std::cerr << "設置RSA密鑰長度失敗" << std::endl;
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    
    EVP_PKEY* pkey = NULL;
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        std::cerr << "RSA密鑰生成失敗" << std::endl;
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    
    // 保存私鑰
    BIO* bp_private = BIO_new_file(privateKeyFile.c_str(), "w+");
    if (!bp_private) {
        std::cerr << "無法創建私鑰文件" << std::endl;
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    
    if (!PEM_write_bio_PrivateKey(bp_private, pkey, NULL, NULL, 0, NULL, NULL)) {
        std::cerr << "無法寫入私鑰" << std::endl;
        BIO_free_all(bp_private);
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    
    // 保存公鑰
    BIO* bp_public = BIO_new_file(publicKeyFile.c_str(), "w+");
    if (!bp_public) {
        std::cerr << "無法創建公鑰文件" << std::endl;
        BIO_free_all(bp_private);
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    
    if (!PEM_write_bio_PUBKEY(bp_public, pkey)) {
        std::cerr << "無法寫入公鑰" << std::endl;
        BIO_free_all(bp_public);
        BIO_free_all(bp_private);
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    
    // 釋放資源
    BIO_free_all(bp_private);
    BIO_free_all(bp_public);
    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(ctx);
    
    std::cout << "RSA密鑰對生成成功!" << std::endl;
    std::cout << "私鑰已保存到: " << privateKeyFile << std::endl;
    std::cout << "公鑰已保存到: " << publicKeyFile << std::endl;
    
    return true;
}

int main(int argc, char* argv[]) {
    // Windows console UTF-8 support
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // 初始化OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string command = argv[1];

    if (command == "sign" && argc == 5) {
        std::string inputFilename = argv[2];
        std::string outputFilename = argv[3];
        std::string privateKeyPath = argv[4];

        std::vector<std::string> lines = readCSV(inputFilename);
        if (lines.empty()) {
            return 1;
        }

        std::string contentHash = calculateHash(lines);
        std::cout << "文件內容哈希: " << contentHash << std::endl;

        std::string signature = signData(contentHash, privateKeyPath);
        if (signature.empty()) {
            std::cerr << "簽名生成失敗" << std::endl;
            return 1;
        }

        std::cout << "簽名: " << signature << std::endl;

        if (appendSignatureToCSV(inputFilename, outputFilename, signature)) {
            std::cout << "簽名已成功追加到文件: " << outputFilename << std::endl;
            return 0;
        } else {
            std::cerr << "無法將簽名追加到文件" << std::endl;
            return 1;
        }
    } else if (command == "verify" && argc == 4) {
        std::string filename = argv[2];
        std::string publicKeyPath = argv[3];

        std::vector<std::string> lines = readCSV(filename);
        if (lines.size() < 2) {
            std::cerr << "文件格式無效或沒有簽名行" << std::endl;
            return 1;
        }

        // 獲取簽名行並解析
        std::string signatureLine = lines.back();
        lines.pop_back(); // 移除簽名行以獲取原始內容

        size_t pos = signatureLine.find("|");
        if (pos == std::string::npos || signatureLine.substr(0, pos) != "SIGNATURE") {
            std::cerr << "無效的簽名行格式" << std::endl;
            return 1;
        }

        std::string signature = signatureLine.substr(pos + 1);
        
        std::string contentHash = calculateHash(lines);
        std::cout << "文件內容哈希: " << contentHash << std::endl;

        if (verifySignature(contentHash, signature, publicKeyPath)) {
            std::cout << "簽名驗證成功!" << std::endl;
            return 0;
        } else {
            std::cerr << "簽名驗證失敗!" << std::endl;
            return 1;
        }
    } else if (command == "genkey" && argc == 4) {
        std::string privateKeyFile = argv[2];
        std::string publicKeyFile = argv[3];
        
        if (generateRSAKeyPair(privateKeyFile, publicKeyFile)) {
            return 0;
        } else {
            return 1;
        }
    } else {
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
