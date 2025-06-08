#ifndef STACK_STRING_H
#define STACK_STRING_H

#include <stddef.h>

template<size_t N>
class stack_string {
public:
    using value_type = char;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using reference = char&;
    using const_reference = const char&;
    using pointer = char*;
    using const_pointer = const char*;
    using iterator = char*;
    using const_iterator = const char*;
    
    static constexpr size_type npos = static_cast<size_type>(-1);

private:
    char data_[N + 1];  // +1 for null terminator
    size_type size_;
    
    // Helper function to get string length (since we can't use strlen)
    static size_type string_length(const char* str) {
        if (!str) return 0;
        size_type len = 0;
        while (str[len] != '\0' && len < N) {
            ++len;
        }
        return len;
    }
    
    // Helper function to copy characters
    void copy_chars(const char* src, size_type count) {
        for (size_type i = 0; i < count && i < N; ++i) {
            data_[i] = src[i];
        }
    }
    
    // Helper function to compare strings
    static int compare_strings(const char* lhs, const char* rhs, size_type count) {
        for (size_type i = 0; i < count; ++i) {
            if (lhs[i] < rhs[i]) return -1;
            if (lhs[i] > rhs[i]) return 1;
            if (lhs[i] == '\0') return 0;
        }
        return 0;
    }

public:
    // Constructors
    stack_string() : size_(0) {
        data_[0] = '\0';
    }
    
    stack_string(const char* str) : size_(0) {
        if (str) {
            size_ = string_length(str);
            copy_chars(str, size_);
        }
        data_[size_] = '\0';
    }
    
    stack_string(const char* str, size_type count) : size_(0) {
        if (str && count > 0) {
            size_ = (count < N) ? count : N;
            copy_chars(str, size_);
        }
        data_[size_] = '\0';
    }
    
    explicit stack_string(size_type count, char ch) : size_((count < N) ? count : N) {
        for (size_type i = 0; i < size_; ++i) {
            data_[i] = ch;
        }
        data_[size_] = '\0';
    }
    
    stack_string(const stack_string& other) : size_(other.size_) {
        copy_chars(other.data_, size_);
        data_[size_] = '\0';
    }
    
    template<size_t M>
    stack_string(const stack_string<M>& other) : size_(0) {
        size_type copy_size = (other.size() < N) ? other.size() : N;
        copy_chars(other.c_str(), copy_size);
        size_ = copy_size;
        data_[size_] = '\0';
    }
    
    // Assignment operators
    stack_string& operator=(const stack_string& other) {
        if (this != &other) {
            size_ = other.size_;
            copy_chars(other.data_, size_);
            data_[size_] = '\0';
        }
        return *this;
    }
    
    stack_string& operator=(const char* str) {
        if (str) {
            size_ = string_length(str);
            copy_chars(str, size_);
        } else {
            size_ = 0;
        }
        data_[size_] = '\0';
        return *this;
    }
    
    stack_string& operator=(char ch) {
        size_ = 1;
        data_[0] = ch;
        data_[1] = '\0';
        return *this;
    }
    
    // Element access
    reference at(size_type pos) {
        return data_[pos];  // In embedded environment, might skip bounds checking
    }
    
    const_reference at(size_type pos) const {
        return data_[pos];
    }
    
    reference operator[](size_type pos) {
        return data_[pos];
    }
    
    const_reference operator[](size_type pos) const {
        return data_[pos];
    }
    
    reference front() {
        return data_[0];
    }
    
    const_reference front() const {
        return data_[0];
    }
    
    reference back() {
        return data_[size_ - 1];
    }
    
    const_reference back() const {
        return data_[size_ - 1];
    }
    
    const char* c_str() const {
        return data_;
    }
    
    const char* data() const {
        return data_;
    }
    
    // Iterators
    iterator begin() {
        return data_;
    }
    
    const_iterator begin() const {
        return data_;
    }
    
    const_iterator cbegin() const {
        return data_;
    }
    
    iterator end() {
        return data_ + size_;
    }
    
    const_iterator end() const {
        return data_ + size_;
    }
    
    const_iterator cend() const {
        return data_ + size_;
    }
    
    // Capacity
    bool empty() const {
        return size_ == 0;
    }
    
    size_type size() const {
        return size_;
    }
    
    size_type length() const {
        return size_;
    }
    
    constexpr size_type max_size() const {
        return N;
    }
    
    constexpr size_type capacity() const {
        return N;
    }
    
    // Modifiers
    void clear() {
        size_ = 0;
        data_[0] = '\0';
    }
    
    stack_string& append(const char* str) {
        if (str) {
            size_type str_len = string_length(str);
            size_type available = N - size_;
            size_type append_len = (str_len < available) ? str_len : available;
            
            //copy_chars(str, append_len);
            for (size_type i = 0; i < append_len; ++i) {
                data_[size_ + i] = str[i];
            }
            size_ += append_len;
            data_[size_] = '\0';
        }
        return *this;
    }
    
    stack_string& append(const char* str, size_type count) {
        if (str && count > 0) {
            size_type available = N - size_;
            size_type append_len = (count < available) ? count : available;
            
            for (size_type i = 0; i < append_len; ++i) {
                data_[size_ + i] = str[i];
            }
            size_ += append_len;
            data_[size_] = '\0';
        }
        return *this;
    }
    
    stack_string& append(size_type count, char ch) {
        size_type available = N - size_;
        size_type append_len = (count < available) ? count : available;
        
        for (size_type i = 0; i < append_len; ++i) {
            data_[size_ + i] = ch;
        }
        size_ += append_len;
        data_[size_] = '\0';
        return *this;
    }
    
    template<size_t M>
    stack_string& append(const stack_string<M>& other) {
        return append(other.c_str(), other.size());
    }
    
    void push_back(char ch) {
        if (size_ < N) {
            data_[size_] = ch;
            ++size_;
            data_[size_] = '\0';
        }
    }
    
    void pop_back() {
        if (size_ > 0) {
            --size_;
            data_[size_] = '\0';
        }
    }
    
    stack_string& erase(size_type pos = 0, size_type len = npos) {
        if (pos >= size_) return *this;
        
        size_type actual_len = (len == npos || pos + len > size_) ? size_ - pos : len;
        size_type chars_to_move = size_ - pos - actual_len;
        
        // Move characters after the erased section
        for (size_type i = 0; i < chars_to_move; ++i) {
            data_[pos + i] = data_[pos + actual_len + i];
        }
        
        size_ -= actual_len;
        data_[size_] = '\0';
        return *this;
    }
    
    iterator erase(const_iterator position) {
        if (position >= begin() && position < end()) {
            size_type pos = position - begin();
            erase(pos, 1);
            return begin() + pos;
        }
        return end();
    }
    
    iterator erase(const_iterator first, const_iterator last) {
        if (first >= begin() && first <= end() && last >= first && last <= end()) {
            size_type pos = first - begin();
            size_type len = last - first;
            erase(pos, len);
            return begin() + pos;
        }
        return end();
    }
    
    // String operations
    size_type find(char ch, size_type pos = 0) const {
        for (size_type i = pos; i < size_; ++i) {
            if (data_[i] == ch) {
                return i;
            }
        }
        return npos;
    }
    
    size_type find(const char* str, size_type pos = 0) const {
        if (!str || pos >= size_) return npos;
        
        size_type str_len = string_length(str);
        if (str_len == 0) return pos;
        if (str_len > size_ - pos) return npos;
        
        for (size_type i = pos; i <= size_ - str_len; ++i) {
            bool found = true;
            for (size_type j = 0; j < str_len; ++j) {
                if (data_[i + j] != str[j]) {
                    found = false;
                    break;
                }
            }
            if (found) return i;
        }
        return npos;
    }
    
    size_type rfind(char ch, size_type pos = npos) const {
        if (size_ == 0) return npos;
        
        size_type start_pos = (pos == npos || pos >= size_) ? size_ - 1 : pos;
        
        for (size_type i = start_pos + 1; i > 0; --i) {
            if (data_[i - 1] == ch) {
                return i - 1;
            }
        }
        return npos;
    }
    
    size_type rfind(const char* str, size_type pos = npos) const {
        if (!str) return npos;
        
        size_type str_len = string_length(str);
        if (str_len == 0) return (pos < size_) ? pos : size_;
        if (str_len > size_) return npos;
        
        size_type start_pos = (pos == npos || pos > size_ - str_len) ? size_ - str_len : pos;
        
        for (size_type i = start_pos + 1; i > 0; --i) {
            bool found = true;
            for (size_type j = 0; j < str_len; ++j) {
                if (data_[i - 1 + j] != str[j]) {
                    found = false;
                    break;
                }
            }
            if (found) return i - 1;
        }
        return npos;
    }
    
    size_type find_first_not_of(char ch, size_type pos = 0) const {
        for (size_type i = pos; i < size_; ++i) {
            if (data_[i] != ch) {
                return i;
            }
        }
        return npos;
    }
    
    size_type find_first_not_of(const char* str, size_type pos = 0) const {
        if (!str || pos >= size_) return npos;
        
        for (size_type i = pos; i < size_; ++i) {
            bool found_in_str = false;
            for (const char* p = str; *p != '\0'; ++p) {
                if (data_[i] == *p) {
                    found_in_str = true;
                    break;
                }
            }
            if (!found_in_str) {
                return i;
            }
        }
        return npos;
    }
    
    size_type find_last_not_of(char ch, size_type pos = npos) const {
        if (size_ == 0) return npos;
        
        size_type start_pos = (pos == npos || pos >= size_) ? size_ - 1 : pos;
        
        for (size_type i = start_pos + 1; i > 0; --i) {
            if (data_[i - 1] != ch) {
                return i - 1;
            }
        }
        return npos;
    }
    
    size_type find_last_not_of(const char* str, size_type pos = npos) const {
        if (!str || size_ == 0) return npos;
        
        size_type start_pos = (pos == npos || pos >= size_) ? size_ - 1 : pos;
        
        for (size_type i = start_pos + 1; i > 0; --i) {
            bool found_in_str = false;
            for (const char* p = str; *p != '\0'; ++p) {
                if (data_[i - 1] == *p) {
                    found_in_str = true;
                    break;
                }
            }
            if (!found_in_str) {
                return i - 1;
            }
        }
        return npos;
    }
    
    stack_string substr(size_type pos = 0, size_type len = npos) const {
        if (pos >= size_) return stack_string();
        
        size_type actual_len = (len == npos || pos + len > size_) ? size_ - pos : len;
        return stack_string(data_ + pos, actual_len);
    }

    void reserve(size_type len) const {
    }
    
    // Operators
    stack_string& operator+=(const stack_string& other) {
        return append(other.c_str(), other.size());
    }
    
    stack_string& operator+=(const char* str) {
        return append(str);
    }
    
    stack_string& operator+=(char ch) {
        push_back(ch);
        return *this;
    }
    
    // Comparison operators
    bool operator==(const stack_string& other) const {
        if (size_ != other.size_) return false;
        return compare_strings(data_, other.data_, size_) == 0;
    }
    
    bool operator==(const char* str) const {
        if (!str) return size_ == 0;
        size_type str_len = string_length(str);
        if (size_ != str_len) return false;
        return compare_strings(data_, str, size_) == 0;
    }
    
    bool operator!=(const stack_string& other) const {
        return !(*this == other);
    }
    
    bool operator!=(const char* str) const {
        return !(*this == str);
    }
    
    bool operator<(const stack_string& other) const {
        size_type min_size = (size_ < other.size_) ? size_ : other.size_;
        int result = compare_strings(data_, other.data_, min_size);
        return result < 0 || (result == 0 && size_ < other.size_);
    }
    
    bool operator<=(const stack_string& other) const {
        return *this < other || *this == other;
    }
    
    bool operator>(const stack_string& other) const {
        return other < *this;
    }
    
    bool operator>=(const stack_string& other) const {
        return !(*this < other);
    }
};

// Non-member operators
template<size_t N>
stack_string<N> operator+(const stack_string<N>& lhs, const stack_string<N>& rhs) {
    stack_string<N> result(lhs);
    result += rhs;
    return result;
}

template<size_t N>
stack_string<N> operator+(const stack_string<N>& lhs, const char* rhs) {
    stack_string<N> result(lhs);
    result += rhs;
    return result;
}

template<size_t N>
stack_string<N> operator+(const char* lhs, const stack_string<N>& rhs) {
    stack_string<N> result(lhs);
    result += rhs;
    return result;
}

template<size_t N>
stack_string<N> operator+(const stack_string<N>& lhs, char rhs) {
    stack_string<N> result(lhs);
    result += rhs;
    return result;
}

template<size_t N>
stack_string<N> operator+(char lhs, const stack_string<N>& rhs) {
    stack_string<N> result(1, lhs);
    result += rhs;
    return result;
}

// Equality operators for const char* on the left
template<size_t N>
bool operator==(const char* lhs, const stack_string<N>& rhs) {
    return rhs == lhs;
}

template<size_t N>
bool operator!=(const char* lhs, const stack_string<N>& rhs) {
    return rhs != lhs;
}

#endif
