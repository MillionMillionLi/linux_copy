// --- Public Input Method Implementations (Normal Version) ---

void PackedShareBundle::input_from_file(int player_no, int degree)
{
    set_degree(degree); // Set and validate degree first
    if (player_no ==ShareBase::P->my_num()) {
        if (packing_factor_k <= 0) throw std::runtime_error("Packing factor k not set");
        // Ensure secrets matrix is sized correctly
        if(secrets.rows() != original_rows || secrets.cols() != original_cols || num_secrets_total == 0) {
            // If dimensions aren't set or don't match, try to resize based on current R, C
            // This assumes R, C were set correctly by constructor or resize()
            secrets.resize(original_rows, original_cols);
            update_dimensions(); // Recalculate sizes
            if(num_secrets_total == 0 && (original_rows > 0 || original_cols > 0)) {
                 throw std::runtime_error("Cannot determine secrets size for input_from_file");
            } else if (num_secrets_total == 0) {
                 return; // Nothing to read if size is 0x0
            }
        }

        for(size_t i = 0; i < num_secrets_total; ++i) {
            if (!(ShareBase::in >> secrets(i))) { // Use static 'in' stream
                 throw file_error("Error reading secret from file for PackedShareBundle");
            }
        }
        distribute_sharings(); // Call protected method
    } else {
        receive_shares(player_no); // Call protected method
    }
}

void PackedShareBundle::input_from_party(int player_no, int degree)
{
    set_degree(degree);
    if (player_no ==ShareBase::P->my_num()) {
         // Distributor needs to have secrets set *before* calling this
         if (num_secrets_total != static_cast<size_t>(secrets.size())) {
             throw std::runtime_error("PackedShareBundle::input_from_party: Secrets not set or size mismatch before distributing.");
         }
        distribute_sharings(); // Call protected method
    } else {
        receive_shares(player_no); // Call protected method
    }
}

void PackedShareBundle::input_from_pking(int degree)
{
    input_from_party(ShareBase::Pking, degree); // Use static Pking
}

void PackedShareBundle::input_from_random(int player_no, int degree)
{
    set_degree(degree);
    if (player_no ==ShareBase::P->my_num()) {
        // Ensure secrets matrix is sized correctly
        if(secrets.rows() != original_rows || secrets.cols() != original_cols || num_secrets_total == 0) {
            secrets.resize(original_rows, original_cols);
            update_dimensions();
             if(num_secrets_total == 0 && (original_rows > 0 || original_cols > 0)) {
                 throw std::runtime_error("Cannot determine secrets size for input_from_random");
             } else if (num_secrets_total == 0) {
                  return;
             }
        }
        random_matrix(secrets); // Fill R x C secrets matrix
        distribute_sharings(); // Call protected method
    } else {
        receive_shares(player_no); // Call protected method
    }
}

// --- Public Input Method Implementations (Multi-Thread Version) ---

void PackedShareBundle::input_from_file_request(int player_no, int degree, octetStreams &os_send, octetStream &o_receive)
{
    set_degree(degree); // Set degree first
    if (player_no ==ShareBase::P->my_num()) {
        // Read secrets (similar logic to blocking version)
        if (packing_factor_k <= 0) throw std::runtime_error("Packing factor k not set");
        if(secrets.rows() != original_rows || secrets.cols() != original_cols || num_secrets_total == 0) {
            secrets.resize(original_rows, original_cols); update_dimensions();
             if(num_secrets_total == 0 && (original_rows > 0 || original_cols > 0)) throw std::runtime_error("Cannot determine secrets size");
             else if (num_secrets_total == 0) return;
        }
        for(size_t i = 0; i < num_secrets_total; ++i) { if (!(ShareBase::in >> secrets(i))) throw file_error("Error reading secret"); }

        // Calculate shares and request send
        calculate_sharings_internal(os_send); // Fills os_send and this->packed_shares
       ShareBase::P->request_send_respective_non_empty(os_send);
    } else {
        // Request receive
       ShareBase::P->request_receive(player_no, o_receive);
    }
}

void PackedShareBundle::input_from_party_request(int player_no, int degree, octetStreams &os_send, octetStream &o_receive)
{
    set_degree(degree);
    if (player_no ==ShareBase::P->my_num()) {
        if (num_secrets_total != static_cast<size_t>(secrets.size())) throw std::runtime_error("Secrets not set/mismatch");
        calculate_sharings_internal(os_send);
       ShareBase::P->request_send_respective_non_empty(os_send);
    } else {
       ShareBase::P->request_receive(player_no, o_receive);
    }
}

void PackedShareBundle::input_from_random_request(int player_no, int degree, octetStreams &os_send, octetStream &o_receive)
{
    set_degree(degree);
    if (player_no ==ShareBase::P->my_num()) {
        if(secrets.rows() != original_rows || secrets.cols() != original_cols || num_secrets_total == 0) {
            secrets.resize(original_rows, original_cols); update_dimensions();
             if(num_secrets_total == 0 && (original_rows > 0 || original_cols > 0)) throw std::runtime_error("Cannot determine secrets size");
             else if (num_secrets_total == 0) return;
        }
        random_matrix(secrets);
        calculate_sharings_internal(os_send);
       ShareBase::P->request_send_respective_non_empty(os_send);
    } else {
       ShareBase::P->request_receive(player_no, o_receive);
    }
}

void PackedShareBundle::finish_input_from(int player_no, octetStreams &os_send, octetStream &o_receive)
{
    if (player_no ==ShareBase::P->my_num()) {
        // Distributor waits for send completion
       ShareBase::P->wait_send_respective_non_empty(os_send);
    } else {
        // Receiver waits for receive completion
       ShareBase::P->wait_receive(player_no, o_receive);
        // Ensure packed_shares is ready to be unpacked into
        if (num_packed_shares == 0) update_dimensions();
        if (packed_shares.size() == 0 && num_packed_shares > 0) packed_shares.resize(num_packed_shares, 1);

        if (static_cast<size_t>(packed_shares.size()) * sizeof(gfpScalar) > o_receive.get_length()) {
             throw std::runtime_error("Received insufficient data for packed shares in finish_input_from.");
        }
        if (packed_shares.size() > 0) {
            unpack_matrix(packed_shares, o_receive); // Unpack received packed shares
        } else if (o_receive.get_length() > 0) {
             std::cerr << "Warning: Received data in finish_input_from but expected zero packed shares." << std::endl;
             o_receive.clear();
        }
    }
}


// --- Public Input Methods (Multi-Thread using Static Buffers) ---

void PackedShareBundle::input_from_file_request(int player_no, int degree)
{
    if(player_no ==ShareBase::P->my_num()){ ShareBase::send_buffers.clear(); }
    else{ ShareBase::receive_buffers[player_no].clear(); }
    input_from_file_request(player_no, degree, ShareBase::send_buffers, ShareBase::receive_buffers[player_no]);
}

void PackedShareBundle::input_from_party_request(int player_no, int degree)
{
    if(player_no ==ShareBase::P->my_num()){ ShareBase::send_buffers.clear(); }
    else{ ShareBase::receive_buffers[player_no].clear(); }
    input_from_party_request(player_no, degree, ShareBase::send_buffers, ShareBase::receive_buffers[player_no]);
}

void PackedShareBundle::input_from_pking_request(int degree)
{
    input_from_party_request(ShareBase::Pking, degree); // Delegate, uses static buffers internally
}

void PackedShareBundle::input_from_random_request(int player_no, int degree)
{
    if(player_no ==ShareBase::P->my_num()){ ShareBase::send_buffers.clear(); }
    else{ ShareBase::receive_buffers[player_no].clear(); }
    input_from_random_request(player_no, degree, ShareBase::send_buffers, ShareBase::receive_buffers[player_no]);
}

void PackedShareBundle::finish_input_from(int player_no)
{
    finish_input_from(player_no, ShareBase::send_buffers, ShareBase::receive_buffers[player_no]);
}

void PackedShareBundle::finish_input_from_pking()
{
    finish_input_from(ShareBase::Pking); // Delegate
}
