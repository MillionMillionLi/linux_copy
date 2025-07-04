#include <iostream>
#include <vector>
#include <cassert> // For assertions
#include <exception> // For std::exception

// --- 核心依赖 ---
#include "Protocols/PackedShareBase.h" // 我们要测试的类
#include "Math/gfpScalar.h"          // 底层数据类型
#include "Math/gfpMatrix.h"          // 底层矩阵类型
#include "Protocols/Share.h"           // 需要访问 ShareBase::threshold (间接包含 Player.h 等)
                                       // 注意：这里仍然包含了 Share.h，它可能间接包含
                                       // Player.h 等。如果 Share.h 本身依赖过多，
                                       // 可能需要创建一个更 minimal 的头文件只包含 ShareBase 定义。
                                       // 但为了简单起见，我们先假设包含 Share.h 是OK的。


// --- 全局设置 (替代复杂的初始化) ---
// 在 main 函数开始前设置 ShareBase 的静态变量
void setup_minimal_sharebase(int n, int t) {
    hmmpc::ShareBase::n_players = n;
    hmmpc::ShareBase::threshold = t;
    // 将 ShareBase::P 设置为 nullptr，因为我们不测试需要 Player 的功能
    hmmpc::ShareBase::P = nullptr;
    hmmpc::ShareBase::Pking = 0; // 设置一个默认值

    // 注意：这里没有调用 ShareBase::init_vandermondes() 等，
    // 因为我们的测试目前不依赖这些矩阵。如果后续测试需要，再添加。
    // 同样，没有设置 PRNG，通信缓冲区等。
    std::cout << "[Minimal Setup] ShareBase::n_players = " << hmmpc::ShareBase::n_players << std::endl;
    std::cout << "[Minimal Setup] ShareBase::threshold = " << hmmpc::ShareBase::threshold << std::endl;
}

int main() {
    using namespace hmmpc;

    // --- 测试参数 ---
    int n = 7; // Example: Number of players (must be >= D+1)
    int t = 3; // Example: SSS threshold (t = (n-1)/2 for n=7)
    int k = 2; // Example: Packing factor
    // Determine PSS degree D. Common choice: D = t


    std::cout << "--- PackedShareBase Core Logic Unit Test ---" << std::endl;
    std::cout << "n = " << n << ", SSS t = " << t << ", k = " << k << std::endl;

    // --- 1. Minimal ShareBase Setup ---
    // 直接设置静态变量，避免复杂的对象创建和网络
    setup_minimal_sharebase(n, t);
    std::cout << "Minimal ShareBase setup complete." << std::endl;

    // --- 2. Initialize PackedShareBase ---
    try {
        // 调用初始化函数，它会设置 k, D, t' 并预计算矩阵
        PackedShareBase::initiate_packed_protocol(k);
        std::cout << "PackedShareBase initiated successfully." << std::endl;
        std::cout << "  Packing factor (k): " << PackedShareBase::packing_factor_k << std::endl;
        std::cout << "  PSS Privacy Threshold (t'): " << PackedShareBase::pss_threshold << std::endl;
        std::cout << "  Secret Embedding Points: ";
        for(const auto& p : PackedShareBase::secret_embedding_points) { std::cout << p << " "; }
        std::cout << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "PackedShareBase initiation failed: " << e.what() << std::endl;
        return 1; // 初始化失败，测试无法继续
    }

    // --- 3. Test Secret Reconstruction Matrix (reconstruction_matrix_t) ---
    std::cout << "\nTesting Secret Reconstruction Matrix (R)..." << std::endl;
    try {
        // 直接访问静态成员获取预计算的矩阵
        const gfpMatrix& R = PackedShareBase::reconstruction_matrix_t;

        std::cout << "  Dimensions (k x D+1): " << R.rows() << " x " << R.cols() << std::endl;
        assert(R.rows() == k);
        assert(R.cols() == t + 1);
        std::cout << "  Matrix R:\n" << R << std::endl; // Optional print
        
        const gfpMatrix& R_2t = PackedShareBase::reconstruction_matrix_2t;

        std::cout << "  Dimensions (k x 2t+1): " << R_2t.rows() << " x " << R_2t.cols() << std::endl;
        assert(R_2t.rows() == k);
        assert(R_2t.cols() == 2 * t + 1);
        std::cout << "  Matrix R_2t:\n" << R_2t << std::endl; // Optional print
        // --- Verification Logic (Manual Polynomial) ---
        // Create a known polynomial f(x) of degree D = t.
        // Example: k=2, t=3, D=3. Secrets at p0=-1, p1=0. Shares at 1, 2, 3, 4.
        // f(x) = 5 + 2x + x^2 + 3x^3 (degree D=3)

        // Calculate expected secrets based on the polynomial at embedding points
        gfpVector secrets_expected(k);
        gfpScalar x_val;
        // s0 = f(-1) = f(secret_embedding_points[0])
        x_val = PackedShareBase::secret_embedding_points[0]; // Should be -1 mapped to gfp
        secrets_expected(0) = gfpScalar(5) + gfpScalar(2)*x_val + x_val*x_val + gfpScalar(3)*x_val*x_val*x_val;
        // s1 = f(0) = f(secret_embedding_points[1])
        x_val = PackedShareBase::secret_embedding_points[1]; // Should be 0 mapped to gfp
        secrets_expected(1) = gfpScalar(5) + gfpScalar(2)*x_val + x_val*x_val + gfpScalar(3)*x_val*x_val*x_val;


        // Calculate shares based on the polynomial at reconstruction points (1 to D+1)
        gfpVector shares_calculated(t + 1);
        for (int j = 0; j <= t; ++j) {
            gfpScalar alpha_j = gfpScalar(j + 1); // Point j+1
            shares_calculated(j) = gfpScalar(5) + gfpScalar(2)*alpha_j + alpha_j*alpha_j + gfpScalar(3)*alpha_j*alpha_j*alpha_j;
        }

        // Reconstruct secrets using the matrix
        gfpVector secrets_reconstructed = R * shares_calculated;

        std::cout << "  Expected Secrets (f(-1), f(0)):   " << secrets_expected.transpose() << std::endl;
        std::cout << "  Calculated Shares (f(1..4)):  " << shares_calculated.transpose() << std::endl;
        std::cout << "  Reconstructed Secrets: " << secrets_reconstructed.transpose() << std::endl;
        // Verify
        bool success = true;
        for(int i=0; i<k; ++i) {
            // Use a small tolerance if comparing floating point, but gfpScalar should be exact
            if (secrets_reconstructed(i) != secrets_expected(i)) {
                success = false;
                std::cerr << "  Mismatch at secret index " << i << ": Expected " << secrets_expected(i)
                          << ", Got " << secrets_reconstructed(i) << std::endl;
            }
        }
        assert(success && "Secret reconstruction matrix failed verification!");
        std::cout << "  Secret Reconstruction Verification: " << (success ? "PASSED" : "FAILED") << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error during Secret Reconstruction Matrix test: " << e.what() << std::endl;
        return 1;
    }


    // --- 4. Test PRG Reconstruction Matrices (Optional, requires careful verification) ---
    std::cout << "\nTesting PRG Reconstruction Matrix (2t case)..." << std::endl;
    try {
        const gfpMatrix& R_prg_2t = PackedShareBase::Packed_reconstruction_with_secret_2t;
        if (R_prg_2t.rows() > 0 && R_prg_2t.cols() > 0) {
            std::cout << "  Dimensions (k x 2t+1): " << R_prg_2t.rows() << " x " << R_prg_2t.cols() << std::endl;
            // Reminder: Need to use ShareBase::threshold here
            assert(R_prg_2t.rows() == k);
            assert(R_prg_2t.cols() == 2 * ShareBase::threshold + 1); // Use ShareBase::threshold

            // --- Verification Logic (Conceptual) ---
            // 1. Choose polynomial f(x) of degree 2t.
            int degree_2t = 2 * ShareBase::threshold;
            // Example: f(x) = random polynomial of degree 2t
            gfpVector coeffs_2t(degree_2t + 1);
            random_matrix(coeffs_2t); // Assuming random_matrix is available and works

            // 2. Define known values: secrets f(-k+1)..f(0) and shares f(k+1)..f(2t+1)
            gfpVector known_vals(R_prg_2t.cols());
            std::vector<gfpScalar> basis_points_2t; // Recreate basis points for verification
            basis_points_2t.reserve(R_prg_2t.cols());
            basis_points_2t.insert(basis_points_2t.end(), PackedShareBase::secret_embedding_points.begin(), PackedShareBase::secret_embedding_points.end());
            for (int j = k + 1; j <= degree_2t + 1; ++j) { basis_points_2t.push_back(gfpScalar(j)); }

            for(int j=0; j < known_vals.size(); ++j) {
                gfpScalar x_val = basis_points_2t[j];
                gfpScalar fx = 0;
                gfpScalar x_pow = 1;
                for(int c=0; c <= degree_2t; ++c) { // Evaluate polynomial
                    fx += coeffs_2t(c) * x_pow;
                    x_pow *= x_val;
                }
                known_vals(j) = fx;
            }

            // 3. Calculate target values: shares f(1)..f(k)
            gfpVector target_vals_expected(k);
             std::vector<gfpScalar> target_points_2t(k);
            for(int i=0; i<k; ++i) { target_points_2t[i] = gfpScalar(i+1); }

             for(int i=0; i<k; ++i) {
                gfpScalar x_val = target_points_2t[i];
                gfpScalar fx = 0;
                gfpScalar x_pow = 1;
                 for(int c=0; c <= degree_2t; ++c) { // Evaluate polynomial
                    fx += coeffs_2t(c) * x_pow;
                    x_pow *= x_val;
                }
                target_vals_expected(i) = fx;
            }

            // 4. Check if R_prg_2t * known_vals == target_vals_expected
            gfpVector target_vals_reconstructed = R_prg_2t * known_vals;

            std::cout << "  Expected Targets (f(1..k)): " << target_vals_expected.transpose() << std::endl;
            std::cout << "  Reconstructed Targets:      " << target_vals_reconstructed.transpose() << std::endl;

            bool success_2t = true;
            for(int i=0; i<k; ++i) {
                if (target_vals_reconstructed(i) != target_vals_expected(i)) {
                    success_2t = false;
                     std::cerr << "  Mismatch at PRG 2t target index " << i << std::endl;
                }
            }
            assert(success_2t && "PRG 2t reconstruction matrix failed verification!");
            std::cout << "  PRG 2t Reconstruction Verification: " << (success_2t ? "PASSED" : "FAILED") << std::endl;

        } else {
            std::cout << "  Matrix not computed or invalid dimensions." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error during PRG 2t Matrix test: " << e.what() << std::endl;
        // Continue to next test if possible
    }


    // --- 5. Test PRG Reconstruction Matrix (t case) ---
    std::cout << "\nTesting PRG Reconstruction Matrix (t case)..." << std::endl;
     try {
        const gfpMatrix& R_prg_t = PackedShareBase::Packed_reconstruction_with_secret_t;
        if (R_prg_t.rows() > 0 && R_prg_t.cols() > 0) {
            std::cout << "  Dimensions ((k+t) x t+1): " << R_prg_t.rows() << " x " << R_prg_t.cols() << std::endl;
            // Reminder: Need to use ShareBase::threshold here
            assert(R_prg_t.rows() == k + ShareBase::threshold);
            assert(R_prg_t.cols() == ShareBase::threshold + 1);

            // --- Verification Logic (Conceptual) ---
            // 1. Choose polynomial f(x) of degree t.
            int degree_t = ShareBase::threshold;
            int degree_2t = 2 * ShareBase::threshold; // Needed for target points range
            gfpVector coeffs_t(degree_t + 1);
            random_matrix(coeffs_t);

            // 2. Define known values: secrets f(-k+1)..f(0) and shares f(k+1)..f(t+1)
            gfpVector known_vals_t(R_prg_t.cols());
            std::vector<gfpScalar> basis_points_t; // Recreate basis points
            basis_points_t.reserve(R_prg_t.cols());
            basis_points_t.insert(basis_points_t.end(), PackedShareBase::secret_embedding_points.begin(), PackedShareBase::secret_embedding_points.end());
            for (int j = k + 1; j <= degree_t + 1; ++j) { basis_points_t.push_back(gfpScalar(j)); }

            for(int j=0; j < known_vals_t.size(); ++j) {
                gfpScalar x_val = basis_points_t[j];
                gfpScalar fx = 0; gfpScalar x_pow = 1;
                for(int c=0; c <= degree_t; ++c) { fx += coeffs_t(c) * x_pow; x_pow *= x_val;}
                known_vals_t(j) = fx;
            }

            // 3. Calculate target values: shares f(1)..f(k) and f(t+2)..f(2t+1)
            gfpVector target_vals_expected_t(R_prg_t.rows());
            int target_idx = 0;
             // f(1)..f(k)
             for(int i=1; i<=k; ++i) {
                gfpScalar x_val = gfpScalar(i);
                gfpScalar fx = 0; gfpScalar x_pow = 1;
                for(int c=0; c <= degree_t; ++c) { fx += coeffs_t(c) * x_pow; x_pow *= x_val;}
                target_vals_expected_t(target_idx++) = fx;
             }
             // f(t+2)..f(2t+1)
             for(int i = degree_t + 2; i <= degree_2t + 1; ++i) {
                 gfpScalar x_val = gfpScalar(i);
                 gfpScalar fx = 0; gfpScalar x_pow = 1;
                 for(int c=0; c <= degree_t; ++c) { fx += coeffs_t(c) * x_pow; x_pow *= x_val;}
                 target_vals_expected_t(target_idx++) = fx;
             }

            // 4. Check if R_prg_t * known_vals == target_vals_expected
            gfpVector target_vals_reconstructed_t = R_prg_t * known_vals_t;

            std::cout << "  Expected Targets (f(1..k), f(t+2..2t+1)): " << target_vals_expected_t.transpose() << std::endl;
            std::cout << "  Reconstructed Targets:                   " << target_vals_reconstructed_t.transpose() << std::endl;

            bool success_t = true;
            for(int i=0; i<target_vals_expected_t.size(); ++i) {
                if (target_vals_reconstructed_t(i) != target_vals_expected_t(i)) {
                    success_t = false;
                     std::cerr << "  Mismatch at PRG t target index " << i << std::endl;
                }
            }
            assert(success_t && "PRG t reconstruction matrix failed verification!");
             std::cout << "  PRG t Reconstruction Verification: " << (success_t ? "PASSED" : "FAILED") << std::endl;

        } else {
             std::cout << "  Matrix not computed or invalid dimensions." << std::endl;
        }
     } catch (const std::exception& e) {
        std::cerr << "Error during PRG t Matrix test: " << e.what() << std::endl;
     }


    std::cout << "\n--- PackedShareBase Core Logic Unit Test Finished ---" << std::endl;

    return 0;
}