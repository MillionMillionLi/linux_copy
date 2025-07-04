#include "Protocols/PhaseConfig.h"
#include "Protocols/Bit.h"
#include "Protocols/PackedShare.h"
#include "Protocols/PackedShareBase.h"
#include "Protocols/PackedShareBundle.h"
#include "Protocols/PackedBit.h"
#include "Protocols/PackedRandomShare.h"
#include "Networking/Player.h"
#include "Math/gfpMatrix.h"
#include <vector>
#include <iostream>
#include <cassert>
#include <string>
#include <iomanip>
#include <functional>

using namespace std;
using namespace hmmpc;

// 用于同步的辅助函数，替代ThreadPlayer::Synchronize
void synchronize_players(ThreadPlayer* P) {
    octetStream dummy_stream;
    P->send_all(dummy_stream);
    for (int i = 0; i < P->num_players(); i++) {
        if (i != P->my_num()) {
            octetStream temp;
            P->receive_player(i, temp);
        }
    }
}

// 辅助函数 - 打印测试标题
void print_test_header(const string& test_name) {
    cout << "\n========== 测试: " << test_name << " ==========" << endl;
}

// 辅助函数 - 打印测试结果
void print_test_result(bool success, const string& message = "") {
    if (success) {
        cout << "[成功] ";
    } else {
        cout << "[失败] ";
    }
    
    if (!message.empty()) {
        cout << message;
    }
    cout << endl;
}

// 简单测试——验证PSS初始化是否成功
void test_pss_init(PhaseConfig* phase) {
    print_test_header("PSS初始化");
    
    cout << "打包因子k: " << PackedShareBase::packing_factor_k << endl;
    cout << "PSS阈值t': " << PackedShareBase::pss_threshold << endl;
    
    // 检查秘密嵌入点是否正确初始化
    cout << "秘密嵌入点: ";
    for (size_t i = 0; i < PackedShareBase::secret_embedding_points.size(); i++) {
        cout << PackedShareBase::secret_embedding_points[i] << " ";
    }
    cout << endl;
    
    // 检查重构矩阵是否正确初始化
    if (!PackedShareBase::packed_secret_reconstruction_matrix_t.size()) {
        cout << "ERROR: t-度重构矩阵未初始化" << endl;
        print_test_result(false, "t-度重构矩阵未初始化");
    } else {
        cout << "t-度重构矩阵尺寸: " 
             << PackedShareBase::packed_secret_reconstruction_matrix_t.rows() 
             << "x" 
             << PackedShareBase::packed_secret_reconstruction_matrix_t.cols() 
             << endl;
        print_test_result(true, "所有矩阵已正确初始化");
    }
}

// // 测试基本的PackedShare操作
// void test_packed_share(PhaseConfig* phase) {
//     print_test_header("PackedShare基本操作");
    
//     int degree = ShareBase::threshold; // 使用t度多项式
    
//     // 创建测试数据 - k个秘密
//     gfpVector test_secrets(PackedShareBase::packing_factor_k);
//     for (int i = 0; i < PackedShareBase::packing_factor_k; i++) {
//         test_secrets(i) = i + 10; // 简单的测试值: 10, 11, 12, ...
//     }
    
//     cout << "原始秘密: ";
//     for (int i = 0; i < test_secrets.size(); i++) {
//         cout << test_secrets(i) << " ";
//     }
//     cout << endl;
    
//     // 创建PackedShare并设置秘密
//     PackedShare ps(degree);
//     ps.set_secrets(test_secrets);
    
//     // 分发秘密
//     ps.distribute_share();
//     cout << "秘密已分发" << endl;
    
//     // 重构秘密
//     ps.reconstruct_share();
    
//     // 验证重构结果
//     bool valid = true;
//     for (int i = 0; i < PackedShareBase::packing_factor_k; i++) {
//         if (ps.get_secrets()(i) != test_secrets(i)) {
//             valid = false;
//             cout << "重构错误: 位置 " << i 
//                  << " 预期:" << test_secrets(i)
//                  << " 得到:" << ps.get_secrets()(i) << endl;
//         }
//     }
    
//     print_test_result(valid, "PackedShare 分发与重构");
// }

// // 测试PackedShare的PRG相关操作
// void test_packed_share_prg(PhaseConfig* phase) {
//     print_test_header("PackedShare PRG操作");
    
//     int degree = ShareBase::threshold; // 使用t度多项式
    
//     // 创建测试数据 - k个秘密
//     gfpVector test_secrets(PackedShareBase::packing_factor_k);
//     for (int i = 0; i < PackedShareBase::packing_factor_k; i++) {
//         test_secrets(i) = i + 20; // 简单的测试值: 20, 21, 22, ...
//     }
    
//     // 创建PackedShare并设置秘密
//     PackedShare ps(degree);
//     ps.set_secrets(test_secrets);
    
//     // 使用PRG分发秘密
//     ps.distribute_share_PRG();
//     cout << "秘密已使用PRG分发" << endl;
    
//     // 重构秘密
//     ps.reconstruct_share_PRG();
    
//     // 验证重构结果
//     bool valid = true;
//     for (int i = 0; i < PackedShareBase::packing_factor_k; i++) {
//         if (ps.get_secrets()(i) != test_secrets(i)) {
//             valid = false;
//             cout << "PRG重构错误: 位置 " << i 
//                  << " 预期:" << test_secrets(i)
//                  << " 得到:" << ps.get_secrets()(i) << endl;
//         }
//     }
    
//     print_test_result(valid, "PackedShare PRG分发与重构");
// }

// // 测试PackedShareBundle基本操作
// void test_packed_share_bundle(PhaseConfig* phase) {
//     print_test_header("PackedShareBundle基本操作");
    
//     // 创建矩阵大小
//     size_t rows = 3;
//     size_t cols = 2;
    
//     // 创建PSS包
//     PackedShareBundle bundle(rows, cols);
    
//     // 设置测试数据
//     gfpMatrix test_secrets(rows, cols);
//     for (int i = 0; i < rows; i++) {
//         for (int j = 0; j < cols; j++) {
//             test_secrets(i, j) = i * 10 + j + 1; // 简单值: 1, 2, 11, 12, 21, 22
//         }
//     }
//     bundle.set_secrets(test_secrets);
    
//     cout << "原始秘密矩阵 (" << rows << "x" << cols << "):" << endl;
//     for (int i = 0; i < rows; i++) {
//         for (int j = 0; j < cols; j++) {
//             cout << setw(4) << test_secrets(i, j);
//         }
//         cout << endl;
//     }
    
//     // 设置度数
//     bundle.set_degree(ShareBase::threshold);
    
//     // 分发秘密
//     bundle.distribute_sharings();
//     cout << "PackedShareBundle秘密已分发" << endl;
    
//     // 重构秘密
//     PackedShareBundle reconstructed(rows, cols);
//     bundle.reveal(reconstructed);
    
//     // 验证重构结果
//     bool valid = true;
//     for (int i = 0; i < rows; i++) {
//         for (int j = 0; j < cols; j++) {
//             if (reconstructed.secret()(i, j) != test_secrets(i, j)) {
//                 valid = false;
//                 cout << "重构错误: 位置(" << i << "," << j 
//                      << ") 预期:" << test_secrets(i, j)
//                      << " 得到:" << reconstructed.secret()(i, j) << endl;
//             }
//         }
//     }
    
//     print_test_result(valid, "PackedShareBundle 分发与重构");
// }

// // 测试PackedShareBundle的操作
// void test_packed_bundle_operations(PhaseConfig* phase) {
//     print_test_header("PackedShareBundle 基本运算");
    
//     // 创建两个PSS包
//     size_t rows = 2, cols = 2;
//     PackedShareBundle a(rows, cols);
//     PackedShareBundle b(rows, cols);
    
//     // 设置测试数据
//     gfpMatrix secrets_a(rows, cols);
//     gfpMatrix secrets_b(rows, cols);
    
//     for (int i = 0; i < rows; i++) {
//         for (int j = 0; j < cols; j++) {
//             secrets_a(i, j) = i * 10 + j + 1; // a: 1, 2, 11, 12
//             secrets_b(i, j) = i * 10 + j + 100; // b: 100, 101, 110, 111
//         }
//     }
    
//     a.set_secrets(secrets_a);
//     b.set_secrets(secrets_b);
    
//     // 设置度数
//     a.set_degree(ShareBase::threshold);
//     b.set_degree(ShareBase::threshold);
    
//     // 分发秘密
//     a.distribute_sharings();
//     b.distribute_sharings();
    
//     // 加法测试
//     PackedShareBundle add_result = a + b;
//     PackedShareBundle add_reconstructed(rows, cols);
//     add_result.reveal(add_reconstructed);
    
//     bool add_valid = true;
//     for (int i = 0; i < rows && add_valid; i++) {
//         for (int j = 0; j < cols && add_valid; j++) {
//             if (add_reconstructed.secret()(i, j) != secrets_a(i, j) + secrets_b(i, j)) {
//                 add_valid = false;
//             }
//         }
//     }
//     print_test_result(add_valid, "加法运算");
    
//     // 减法测试
//     PackedShareBundle sub_result = a - b;
//     PackedShareBundle sub_reconstructed(rows, cols);
//     sub_result.reveal(sub_reconstructed);
    
//     bool sub_valid = true;
//     for (int i = 0; i < rows && sub_valid; i++) {
//         for (int j = 0; j < cols && sub_valid; j++) {
//             if (sub_reconstructed.secret()(i, j) != secrets_a(i, j) - secrets_b(i, j)) {
//                 sub_valid = false;
//             }
//         }
//     }
//     print_test_result(sub_valid, "减法运算");
    
//     // 标量乘法测试
//     gfpScalar scalar = 5;
//     PackedShareBundle mul_result = a * scalar;
//     PackedShareBundle mul_reconstructed(rows, cols);
//     mul_result.reveal(mul_reconstructed);
    
//     bool mul_valid = true;
//     for (int i = 0; i < rows && mul_valid; i++) {
//         for (int j = 0; j < cols && mul_valid; j++) {
//             if (mul_reconstructed.secret()(i, j) != secrets_a(i, j) * scalar) {
//                 mul_valid = false;
//             }
//         }
//     }
//     print_test_result(mul_valid, "标量乘法运算");
// }

// // 测试PackedRandomShare生成随机份额
// void test_packed_random_share(PhaseConfig* phase) {
//     print_test_header("PackedRandomShare测试");
    
//     // 使用阶段配置生成随机份额
//     phase->generate_packed_random_sharings(10);
    
//     // 获取一些随机份额
//     gfpScalar rand_share;
//     PackedRandomShare::get_random(PackedRandomShare::queueRandom, rand_share);
//     cout << "获取到随机份额: " << rand_share << endl;
    
//     // 验证随机份额队列不为空
//     bool success = !PackedRandomShare::queueRandom.empty();
//     print_test_result(success, "生成随机份额");
    
//     // 测试随机比特生成 (如果实现了)
//     phase->generate_packed_random_bits(10);
//     success = !PackedRandomShare::queuePackedRandomBit.empty();
    
//     if (success) {
//         gfpScalar rand_bit;
//         PackedRandomShare::get_random(PackedRandomShare::queuePackedRandomBit, rand_bit);
//         cout << "获取到随机比特份额: " << rand_bit << endl;
//     }
//     print_test_result(success, "生成随机比特份额");
// }

// // 测试PackedShareBundle的度数管理
// void test_degree_management(PhaseConfig* phase) {
//     print_test_header("PSS度数管理");
    
//     // 准备随机性用于度数降低操作
//     phase->generate_packed_reduced_random_sharings(5);
    
//     // 创建一个测试包
//     size_t rows = 2, cols = 2;
//     PackedShareBundle bundle(rows, cols);
    
//     // 设置测试数据
//     gfpMatrix test_secrets(rows, cols);
//     for (int i = 0; i < rows; i++) {
//         for (int j = 0; j < cols; j++) {
//             test_secrets(i, j) = i * 10 + j + 30; // 值: 30, 31, 40, 41
//         }
//     }
//     bundle.set_secrets(test_secrets);
    
//     // 设置为2t度
//     int double_t_degree = ShareBase::threshold << 1;
//     bundle.set_degree(double_t_degree);
//     cout << "初始度数: " << bundle.get_degree() << endl;
    
//     // 分发秘密
//     bundle.distribute_sharings();
    
//     // 应用度数降低
//     bundle.reduce_degree();
//     cout << "降低后度数: " << bundle.get_degree() << endl;
    
//     // 验证降低后的度数
//     bool degree_valid = (bundle.get_degree() == ShareBase::threshold);
//     print_test_result(degree_valid, "度数降低到t");
    
//     // 验证内容正确性
//     PackedShareBundle reconstructed(rows, cols);
//     bundle.reveal(reconstructed);
    
//     bool content_valid = true;
//     for (int i = 0; i < rows && content_valid; i++) {
//         for (int j = 0; j < cols && content_valid; j++) {
//             if (reconstructed.secret()(i, j) != test_secrets(i, j)) {
//                 content_valid = false;
//             }
//         }
//     }
//     print_test_result(content_valid, "度数降低后内容正确性");
// }

// // 测试截断操作 (如果已实现)
// void test_truncation(PhaseConfig* phase) {
//     print_test_header("PSS截断测试");
    
//     // 准备随机性用于截断操作
//     phase->generate_packed_truncated_random_sharings(5);
//     phase->generate_packed_reduced_truncated_sharings(5);
    
//     // 创建测试包
//     size_t rows = 2, cols = 2;
//     PackedShareBundle bundle(rows, cols);
    
//     // 创建大数值进行测试
//     gfpMatrix test_secrets(rows, cols);
//     for (int i = 0; i < rows; i++) {
//         for (int j = 0; j < cols; j++) {
//             // 使用较大的值，右移后能看到明显效果
//             test_secrets(i, j) = (i + 1) * (j + 1) * 1024; // 1024, 2048, 2048, 4096
//         }
//     }
//     bundle.set_secrets(test_secrets);
    
//     // 设置度数并分发
//     bundle.set_degree(ShareBase::threshold);
//     bundle.distribute_sharings();
    
//     // 打印原始值
//     cout << "原始值:" << endl;
//     PackedShareBundle original_values(rows, cols);
//     bundle.reveal(original_values);
//     for (int i = 0; i < rows; i++) {
//         for (int j = 0; j < cols; j++) {
//             cout << original_values.secret()(i, j) << " ";
//         }
//         cout << endl;
//     }
    
//     // 执行截断 (右移10位)
//     int precision = 10;
//     bundle.truncate(precision);
    
//     // 获取截断结果
//     PackedShareBundle truncated_values(rows, cols);
//     bundle.reveal(truncated_values);
    
//     // 打印截断后的值
//     cout << "截断后 (右移 " << precision << " 位):" << endl;
//     for (int i = 0; i < rows; i++) {
//         for (int j = 0; j < cols; j++) {
//             cout << truncated_values.secret()(i, j) << " ";
//         }
//         cout << endl;
//     }
    
//     // 验证截断结果
//     bool truncate_valid = true;
//     for (int i = 0; i < rows && truncate_valid; i++) {
//         for (int j = 0; j < cols && truncate_valid; j++) {
//             // 检查是否接近或等于除以2^precision的结果
//             // 由于舍入问题，允许有1的误差
//             gfpScalar expected = test_secrets(i, j) >> precision;
//             gfpScalar actual = truncated_values.secret()(i, j);
//             if (actual != expected && actual != expected - 1 && actual != expected + 1) {
//                 truncate_valid = false;
//                 cout << "截断错误: 位置(" << i << "," << j 
//                      << ") 预期:" << expected
//                      << " 得到:" << actual << endl;
//             }
//         }
//     }
//     print_test_result(truncate_valid, "截断操作");
// }

// // 测试乘法运算 (如果已实现)
// void test_multiplication(PhaseConfig* phase) {
//     print_test_header("PSS乘法测试");
    
//     // 假设需要准备用于乘法的随机化数据
//     phase->generate_packed_reduced_random_sharings(5);
    
//     // 创建两个PSS包
//     size_t rows = 2, cols = 2;
//     PackedShareBundle a(rows, cols);
//     PackedShareBundle b(rows, cols);
    
//     // 设置测试数据
//     gfpMatrix secrets_a(rows, cols);
//     gfpMatrix secrets_b(rows, cols);
    
//     for (int i = 0; i < rows; i++) {
//         for (int j = 0; j < cols; j++) {
//             secrets_a(i, j) = i + j + 1; // a: 1, 2, 2, 3
//             secrets_b(i, j) = i + j + 2; // b: 2, 3, 3, 4
//         }
//     }
    
//     a.set_secrets(secrets_a);
//     b.set_secrets(secrets_b);
    
//     // 设置度数
//     a.set_degree(ShareBase::threshold);
//     b.set_degree(ShareBase::threshold);
    
//     // 分发秘密
//     a.distribute_sharings();
//     b.distribute_sharings();
    
//     try {
//         // 尝试执行乘法
//         PackedShareBundle product = a.mult(b);
        
//         // 重构结果
//         PackedShareBundle product_reconstructed(rows, cols);
//         product.reveal(product_reconstructed);
        
//         // 验证乘法结果
//         bool mul_valid = true;
//         for (int i = 0; i < rows && mul_valid; i++) {
//             for (int j = 0; j < cols && mul_valid; j++) {
//                 if (product_reconstructed.secret()(i, j) != secrets_a(i, j) * secrets_b(i, j)) {
//                     mul_valid = false;
//                     cout << "乘法错误: 位置(" << i << "," << j 
//                          << ") 预期:" << secrets_a(i, j) * secrets_b(i, j)
//                          << " 得到:" << product_reconstructed.secret()(i, j) << endl;
//                 }
//             }
//         }
//         print_test_result(mul_valid, "乘法操作");
//     } catch (const std::exception& e) {
//         cout << "乘法测试异常: " << e.what() << endl;
//         print_test_result(false, "乘法操作 - 未实现或出错");
//     }
// }

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "使用方法: ./PSSTest.x <player_id>" << endl;
        return 1;
    }

    // 解析参数
    int player_no = atoi(argv[1]);
    
    // 设置MPC环境
    int portnum_base = 6000;
    string filename = "Test/HOSTS.example";
    Names player_name = Names(player_no, portnum_base, filename);
    ThreadPlayer P(player_name, "");
    
    // 初始化基础设置
    int threshold = 1; // 假设t=1，可以支持1个恶意方 (对于3方计算)
    
    // 初始化PhaseConfig
    PhaseConfig phase;
    phase.init(P.num_players(), threshold, &P);
    
    // 设置PackedShare的packing factor和初始化必要的数据结构
    PackedShareBase::packing_factor_k = 2; // 假设每个share打包2个secret
    PackedShareBase::initiate_packed_protocol(PackedShareBase::packing_factor_k);
    
    cout << "--- PSS测试开始 ---" << endl;
    cout << "Party ID: " << player_no << endl;
    cout << "总方数: " << P.num_players() << endl;
    cout << "阈值t: " << threshold << endl;
    cout << "打包因子k: " << PackedShareBase::packing_factor_k << endl;
    
    // // 定义测试函数数组
    // vector<std::function<void(PhaseConfig*)>> tests = {
    //     test_pss_init,
    //     // test_packed_share,
    //     // test_packed_share_prg,
    //     // test_packed_share_bundle,
    //     // test_packed_bundle_operations,
    //     // test_packed_random_share,
    //     // test_degree_management,
    //     // test_truncation,
    //     // test_multiplication
    // };
    
    // // 运行测试
    // for (auto& test_func : tests) {
    //     try {
    //         test_func(&phase);
    //         synchronize_players(&P); // 每个测试后同步各方
    //     } catch (const std::exception& e) {
    //         cout << "测试异常: " << e.what() << endl;
    //         synchronize_players(&P); // 发生异常后也要同步
    //     }
    // }
    // 运行测试 - 只运行初始化测试
    test_pss_init(&phase);
    
    cout << "--- PSS测试完成 ---" << endl;
    
    return 0;
}
