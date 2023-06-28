#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) 
    : _output(capacity), _capacity(capacity), first_unacceptable(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (eof) {
        this->_eof = true;
        this->_eofIndex = index + data.size();
    }
    this->first_unread = this->_output.bytes_read();
    this->first_unacceptable = this->first_unread + this->_capacity;
    // string newData = data;
    // size_t newLen = data.size();
    // size_t newIndex = index;

    //对新数据处理，使其在合法的容量内
    seg newSeg = {index, data.size(), data};
    if (newSeg.index >= this->first_unacceptable) return; //整体超出
    if (newSeg.index + newSeg.len > this->first_unacceptable) { //将超出的部分去除
        newSeg.len = this->first_unacceptable - newSeg.index;
        // if (len <= 0) return;
        newSeg.data = data.substr(0, newSeg.len);
    }
    if (newSeg.index < this->first_unassembale) { //截取部分新数据
        if (newSeg.index + newSeg.len <= this->first_unassembale) return; //全部是已读数据
        newSeg.len = newSeg.len + newSeg.index - this->first_unassembale;
        newSeg.data = data.substr(this->first_unassembale - newSeg.index, newSeg.len);
        newSeg.index = this->first_unassembale;
    }

    //数据重复的话，新来的数据会覆盖原来的数据
    //类似于区间合并
    for (auto it = this->_buffer.begin(); it != this->_buffer.end();) {
        auto next_it = ++it;
        it--;
        if ((it->index >= newSeg.index && it->index < newSeg.index + newSeg.len) || // 
            (newSeg.index >= it->index && newSeg.index < it->index + it->len)) {
                size_t new_index = newSeg.index, new_end = newSeg.index + newSeg.len;
                size_t old_index = it->index, old_end = it->index + it->len;
                string newData;
                if (new_index <= old_index && new_end <= old_end) {
                    newData = newSeg.data + it->data.substr(new_end - old_index);
                }else if (new_index <= old_index && new_end >= old_end) {
                    newData = newSeg.data;
                }else if (new_index >= old_index && new_end <= old_end) {
                    newData = it->data.substr(0, new_index - old_index) + newSeg.data + it->data.substr(new_end - old_index);
                }else {
                    newData = it->data.substr(0, new_index - old_index) + newSeg.data;
                }
                newSeg.index = new_index < old_index ? new_index : old_index;
                newSeg.len = (new_end > old_end ? new_end : old_end) - newSeg.index;
                newSeg.data = newData;
                this->_buffer.erase(it);
        }
        it = next_it;
    }
    this->_buffer.insert(newSeg);

    while (!this->_buffer.empty() && this->first_unassembale == this->_buffer.begin()->index) {
        this->_output.write(this->_buffer.begin()->data);
        this->first_unassembale += this->_buffer.begin()->len;
        this->_buffer.erase(this->_buffer.begin());
    }

    if (this->_eof && this->first_unassembale == this->_eofIndex) {
        this->_output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { 
    size_t byte = 0;
    for (auto it = this->_buffer.begin(); it != this->_buffer.end(); it++) {
        byte += it->len;
    }    
    return byte;
}

bool StreamReassembler::empty() const { return unassembled_bytes() == static_cast<size_t>(0); }
