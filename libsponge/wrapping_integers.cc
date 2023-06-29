#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number 64位的绝对序列号
//! \param isn The initial sequence number ISN
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    return WrappingInt32{isn.raw_value() + static_cast<uint32_t>(n)};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number 当前的序列号
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number 上一个流的绝对序列号
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.

//思路：
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    //wrap(checkpoint, isn)为上一次接收到的序列
    //需要走steps步才能到达新序列
    int32_t steps = n.raw_value() - wrap(checkpoint, isn).raw_value();
    int64_t res = static_cast<int64_t>(checkpoint) + steps;
    return res >= 0 ? res : res + (1UL << 32);
}
