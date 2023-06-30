#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _rto{retx_timeout} {}

uint64_t TCPSender::bytes_in_flight() const { return this->_bytes_in_flight; }

void TCPSender::fill_window() {
    if (!this->_syn_sent) {
        this->_syn_sent = true;
        TCPSegment seg;
        seg.header().syn = true;
        send_segment(seg);
        return;
    }
    if (!this->_segments_outstanding.empty() && this->_segments_outstanding.front().header().syn) return;
    if (!this->_stream.buffer_size() && !this->_stream.eof()) return;
    if (this->_fin_sent) return;

    if (this->_recerve_windows_size) {
        while (this->_recerve_free_space) {
            TCPSegment seg;
            size_t payload_size = min({_stream.buffer_size(),
                                       static_cast<size_t>(this->_recerve_free_space),
                                       static_cast<size_t>(TCPConfig::MAX_PAYLOAD_SIZE)});
            seg.payload() = this->_stream.read(payload_size);
            if (this->_stream.eof() && static_cast<size_t>(this->_recerve_free_space) > payload_size) {
                seg.header().fin = true;
                this->_fin_sent = true;
            }
            send_segment(seg);
            if (this->_stream.buffer_empty()) break;
        }
    }else if (this->_recerve_free_space == 0) {
        TCPSegment seg;
        if (this->_stream.eof()) {
            seg.header().fin = true;
            this->_fin_sent = true;
            send_segment(seg);
        }else if (!this->_stream.buffer_empty()) {
            seg.payload() = this->_stream.read(1);
            send_segment(seg);
        }
    }
}

void TCPSender::send_segment(TCPSegment &seg) {
    seg.header().seqno = next_seqno();
    this->_next_seqno += seg.length_in_sequence_space();
    this->_bytes_in_flight += seg.length_in_sequence_space();
    if (this->_syn_sent) {
        this->_recerve_free_space -= seg.length_in_sequence_space();
    }
    this->_segments_out.push(seg);
    this->_segments_outstanding.push(seg);
    if (!this->_timer_running) {
        this->_timer_running = true;
        this->_time_elapsed = 0;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    uint64_t abs_ackno = unwrap(ackno, this->_isn, this->_next_seqno);
    if (!_ackno_valid(abs_ackno)) return;

    this->_recerve_windows_size = window_size;
    this->_recerve_free_space = window_size;

    while (!this->_segments_outstanding.empty()) {
        TCPSegment seg = this->_segments_outstanding.front();
        //已经确定接收，将其中未处理的缓存中删除
        if (unwrap(seg.header().seqno, this->_isn, this->_next_seqno) + seg.length_in_sequence_space() <= abs_ackno) {
            this->_bytes_in_flight -= seg.length_in_sequence_space();
            this->_segments_outstanding.pop();
            this->_time_elapsed = 0;
            this->_rto = this->_initial_retransmission_timeout;
            this->_consecutive_retransmissions = 0;
        } else {
            break;
        }
    }

    //当前可以从管道中读取多少数据
    if (!this->_segments_outstanding.empty()) {
        this->_recerve_free_space = static_cast<uint16_t>(abs_ackno + 
                                    static_cast<uint64_t>(window_size) - 
                                    unwrap(this->_segments_outstanding.front().header().seqno, this->_isn, this->_next_seqno) - 
                                    this->_bytes_in_flight);
    }

    if (!this->_bytes_in_flight) {
        this->_timer_running = false;
    }

    fill_window();
}

bool TCPSender::_ackno_valid(uint64_t abs_ackno) {
    return abs_ackno <= this->_next_seqno && abs_ackno >= unwrap(this->_segments_outstanding.front().header().seqno, this->_isn, this->_next_seqno);
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    if (!this->_timer_running) return;
    this->_time_elapsed += ms_since_last_tick;
    if (this->_time_elapsed >= this->_rto) {
        this->_segments_out.push(this->_segments_outstanding.front());
        if (this->_recerve_windows_size || this->_segments_outstanding.front().header().syn) {
            ++this->_consecutive_retransmissions;
            this->_rto <<= 1;
        }
        this->_time_elapsed = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return this->_consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    this->_segments_out.push(seg);
}
