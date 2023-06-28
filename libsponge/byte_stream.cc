#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//初始化列表的顺序要和声明变量的顺序一致
ByteStream::ByteStream(const size_t capacity) : _capacity(capacity), _buffer(vector<char>(capacity, '\0')) {}

size_t ByteStream::write(const string &data) {
    size_t _writeSize = 0;
    for (const char &c : data) {
        if (this->_capacity - this->_size <= 0) {
            break;
        } else {
            this->_buffer[this->_write % this->_capacity] = c;
            this->_write++;
            this->_size++;
            _writeSize++;
        }
    }
    return _writeSize;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    const size_t _lenght = len <= this->_size ? len : this->_size;
    string _stream = "";
    for (size_t i = 0; i < _lenght; i++) {
        _stream += this->_buffer[(this->_read + i) % this->_capacity];
    }
    return _stream;
}

//管道是先进先出
//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    const size_t _lenght = len <= this->_size ? len : this->_size;
    this->_read += _lenght;
    this->_size -= _lenght;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
string ByteStream::read(const size_t len) {
    string _stream = peek_output(len);
    pop_output(len);
    return _stream;
}


void ByteStream::end_input() {
    this->_inputEnd = true;
}

bool ByteStream::input_ended() const { return this->_inputEnd; }

size_t ByteStream::buffer_size() const { return this->_size; }

bool ByteStream::buffer_empty() const { return this->_size == static_cast<size_t>(0); }

bool ByteStream::eof() const { return this->_inputEnd && buffer_empty(); }

size_t ByteStream::bytes_written() const { return this->_write; }

size_t ByteStream::bytes_read() const { return this->_read; }

size_t ByteStream::remaining_capacity() const { return this->_capacity - this->_size; }
