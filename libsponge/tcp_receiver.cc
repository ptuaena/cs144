#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader hander = seg.header();

    if (!this->_syn) {
        if (!hander.syn) return;
        this->_isn = hander.seqno.raw_value();
        this->_syn = true;
        this->_reassembler.push_substring(seg.payload().copy(), 0, hander.fin);
    }
    
    uint64_t abs_seqno = unwrap(hander.seqno, WrappingInt32(this->_isn), this->_reassembler.get_first_unassembale());
    this->_reassembler.push_substring(seg.payload().copy(), abs_seqno - 1, hander.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (this->_syn) {
        uint64_t ack = stream_out().bytes_written() + 1; //SYN占用1个字节
        if (stream_out().input_ended()) ack++; //如果结束FIN占用1个字节
        return wrap(ack, WrappingInt32(this->_isn));
    }
    return {}; 
}

size_t TCPReceiver::window_size() const { return this->_capacity - stream_out().buffer_size(); }
