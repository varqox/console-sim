// Krzysztof Małysa
#include <array>
#include <cassert>
#include <condition_variable>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <type_traits>
#include <unistd.h>

inline std::string errmsg(int errnum) noexcept {
	std::array<char, 4000> buff;
	auto errcode = std::to_string(errnum);
	std::string res = " - ";
	res += errcode;
	res += ": ";
	res.append(strerror_r(errnum, buff.data(), buff.size()));
	return res;
}

inline auto errmsg() noexcept { return errmsg(errno); }

#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)

#define CONCATENATE_DETAIL(x, y) x##y
#define CONCAT(x, y) CONCATENATE_DETAIL(x, y)

#define throw_assert(expr) \
	((expr) ? (void)0 : throw std::runtime_error(std::string(__FILE__ ":" \
	STRINGIZE(__LINE__) ": ").append(__PRETTY_FUNCTION__).append( \
	": Assertion `" #expr "` failed.")))

template<class T>
constexpr inline auto string_length(const T& x) noexcept -> decltype(x.size()) {
	return x.size();
}

template<class T>
constexpr inline size_t string_length(T* x) noexcept {
	return __builtin_strlen(x);
}

constexpr inline size_t string_length(char) noexcept { return 1; }

template<class T>
constexpr inline auto data(const T& x) noexcept { return x.data(); }

template<class T, size_t N>
constexpr inline auto data(const T (&x)[N]) noexcept { return x; }

constexpr inline auto data(const char* const x) noexcept { return x; }

constexpr inline auto data(char* const x) noexcept { return x; }

constexpr inline const char* data(const char& c) noexcept { return &c; }

template<class T>
constexpr inline auto stringify(T&& x) noexcept -> decltype(std::forward<T>(x))
{
	return std::forward<T>(x);
}

// Allows stringifying integers
constexpr inline auto stringify(bool x) noexcept {
	return (x ? "true" : "false");
}

constexpr inline char stringify(char x) noexcept { return x; }
constexpr inline auto stringify(unsigned char x) noexcept ->
	decltype(std::to_string(x));

constexpr inline auto stringify(short x) noexcept -> decltype(std::to_string(x));
constexpr inline auto stringify(unsigned short x) noexcept ->
	decltype(std::to_string(x));

constexpr inline auto stringify(int x) noexcept -> decltype(std::to_string(x));
constexpr inline auto stringify(unsigned x) noexcept -> decltype(std::to_string(x));
constexpr inline auto stringify(long x) noexcept -> decltype(std::to_string(x));
constexpr inline auto stringify(unsigned long x) noexcept ->
	decltype(std::to_string(x));

constexpr inline auto stringify(long long x) noexcept -> decltype(std::to_string(x));
constexpr inline auto stringify(unsigned long long x) noexcept ->
	decltype(std::to_string(x));

/**
 * @brief Concentrates @p args into std::string
 *
 * @param args std::string-like objects
 *
 * @return concentration of @p args
 */
template<class... Args>
inline std::string concat_tostr(Args&&... args) {
	return [](auto&&... xx) {
		size_t total_length = 0;
		(void)std::initializer_list<int>{
			(total_length += string_length(xx), 0)...
		};

		std::string res;
		res.reserve(total_length);

		(void)std::initializer_list<int>{
			(res += std::forward<decltype(xx)>(xx), 0)...
		};

		return res;
	}(stringify(std::forward<Args>(args))...);
}

// Very useful - includes exception origin
#define THROW(...) throw std::runtime_error(concat_tostr(__VA_ARGS__, \
	" (thrown at " __FILE__ ":" STRINGIZE(__LINE__) ")"))

class SocketStream {
	int sock_fd_;

	template<size_t N>
	class Buffer {
		static constexpr size_t GUARDS = 1; // for tricky hacks
		std::array<char, N + GUARDS> d;
	public:
		size_t pos = 0;
		size_t size = 0;

		char* data() noexcept { return d.data(); }

		const char* data() const noexcept { return d.data(); }

		size_t max_size() const noexcept { return d.size() - GUARDS; }

		char& operator[](size_t n) { return d[n]; }

		const char& operator[](size_t n) const { return d[n]; }
	};

	Buffer<65536> in_buff_;

	void fill_in_buff() {
		if (in_buff_.pos < in_buff_.size)
			return; // Buffer is not empty

		// Gather data
		ssize_t rc = read(sock_fd_, in_buff_.data(), in_buff_.max_size());
		if (rc == -1 and errno == EAGAIN) {
			// Wait for data
			pollfd pfd = {sock_fd_, POLLIN, 0};

			// if (poll(&pfd, 1, POLL_TIMEOUT) <= 0)
			// 	// Timed out

			if (poll(&pfd, 1, -1) == -1) // Wait indefinitely
				THROW("poll()", errmsg());

			rc = read(sock_fd_, in_buff_.data(), in_buff_.max_size());
		}

		if (rc < 0)
			THROW("read()", errmsg());
		else if (rc == 0)
			THROW("No more data to read!");

		in_buff_.size = rc;
		in_buff_.pos = 0;
	}

	static constexpr bool fast_isspace(char c) noexcept { return (c <= ' '); }

	std::string out_buff_;
	// This is for the writer thread
	std::thread writer_thread_;
	std::string sending_buff;
	std::mutex sending_buff_lock;
	std::condition_variable finished_writing, start_writing;
	volatile bool socket_closed = false;

public:
	SocketStream(const std::string& hostname_or_ip, uint16_t port) {
		struct sockaddr_in name;
		name.sin_family = AF_INET;
		// Convert hostname from std::string to numbers
		struct hostent* hostinfo = gethostbyname(hostname_or_ip.data());
		if (not hostinfo)
			THROW("gethostbyname(", hostname_or_ip, ") - ", hstrerror(h_errno));

		name.sin_addr = *((struct in_addr*)hostinfo->h_addr);
		name.sin_port = htons(port);

		sock_fd_ = socket(PF_INET, SOCK_STREAM, 0);
		if (sock_fd_ == -1)
			THROW("socket()", errmsg());

		if (connect(sock_fd_, (sockaddr*)&name, sizeof(struct sockaddr_in)) == -1) {
			auto errnum = errno;
			close(sock_fd_);
			THROW("connect()", errmsg(errnum));
		}

		// Make socket non-blocking
		int flags = fcntl(sock_fd_, F_GETFL, 0);
		if (flags == -1) {
			auto errnum = errno;
			close(sock_fd_);
			THROW("fcntl()", errmsg(errnum));
		}

		if (fcntl(sock_fd_, F_SETFL, flags | O_NONBLOCK)) {
			auto errnum = errno;
			close(sock_fd_);
			THROW("fcntl()", errmsg(errnum));
		}

		writer_thread_ = std::thread(&SocketStream::writer_main, this);
	}

	SocketStream(const SocketStream&) = delete;
	SocketStream(SocketStream&&) = delete;
	SocketStream& operator=(const SocketStream&) = delete;
	SocketStream& operator=(SocketStream&&) = delete;

	char peekchar() {
		fill_in_buff();
		return in_buff_[in_buff_.pos];
	}

	char getchar() {
		fill_in_buff();
		return in_buff_[in_buff_.pos++];
	}

	void ignore_whitespaces() {
		for (;;) {
			fill_in_buff();
			size_t i = in_buff_.pos;
			in_buff_[in_buff_.size] = 'a';
			while (fast_isspace(in_buff_[i]))
				++i;

			in_buff_.pos = i;
			if (i != in_buff_.size)
				break; // Found a received non-whitespace character
		}
	}

	SocketStream& operator>>(char& c) {
		ignore_whitespaces();
		c = in_buff_[in_buff_.pos++];
		return *this;
	}

	template<class T>
	std::enable_if_t<std::is_integral<T>::value, SocketStream&> operator>>(T& x) {
		ignore_whitespaces();

		bool minus = false;
		if (not std::is_unsigned<T>::value and peekchar() == '-') {
			minus = true;
			++in_buff_.pos;
		}

		x = T();
		for (;;) {
			fill_in_buff();
			size_t i = in_buff_.pos;
			in_buff_[in_buff_.size] = '\0';
			while ('0' <= in_buff_[i] and in_buff_[i] <= '9')
				x = x * 10 + (in_buff_[i++] - '0');

			in_buff_.pos = i;
			if (i != in_buff_.size)
				break; // We found a character that is not a digit (and was received)
		}

		if (minus)
			x = -x;

		return *this;
	}

	SocketStream& operator>>(char* s) {
		ignore_whitespaces();

		size_t j = 0;
		for (;;) {
			fill_in_buff();
			size_t i = in_buff_.pos;
			in_buff_[in_buff_.size] = '\0';
			while (not fast_isspace(in_buff_[i]))
				s[j] = in_buff_[i++];

			in_buff_.pos = i;
			if (i != in_buff_.size)
				break; // We found a received whitespace character
		}

		s[j] = '\0';
		return *this;
	}

	SocketStream& operator>>(std::string& s) {
		s.resize(0);
		ignore_whitespaces();

		for (;;) {
			fill_in_buff();
			size_t i = in_buff_.pos;
			in_buff_[in_buff_.size] = '\0';
			while (not fast_isspace(in_buff_[i]))
				++i;

			// This append is much faster
			s.append(in_buff_.data() + in_buff_.pos, in_buff_.data() + i);
			in_buff_.pos = i;
			if (i != in_buff_.size)
				break; // We found a received whitespace character
		}

		return *this;
	}

	// Reads char by char till encountering @p c (which is not read)
	SocketStream& read_till_char(std::string& s, char c) {
		s.resize(0);
		for (;;) {
			fill_in_buff();
			size_t i = in_buff_.pos;
			in_buff_[in_buff_.size] = c;
			while (in_buff_[i] != c)
				++i;

			// Through append it is much faster
			s.append(in_buff_.data() + in_buff_.pos, in_buff_.data() + i);
			in_buff_.pos = i;
			if (i != in_buff_.size)
				break; // We found a received newline character
		}

		return *this;
	}

	// Reads the whole line (does not put '\n' character in the result)
	SocketStream& getline(std::string& s) {
		read_till_char(s, '\n');
		++in_buff_.pos; // Skip the newline character
		return *this;
	}

	/* ============================== Writing ============================== */
	SocketStream& operator<<(char c) {
		out_buff_ += c;
		return *this;
	}

	template<class T>
	std::enable_if_t<std::is_integral<T>::value, SocketStream&> operator<<(T x) {
		if (x == T())
			return *this << '0';

		std::array<char, 64> buff;
		size_t i = buff.size();
		bool minus = false;
		if (not std::is_unsigned<T>::value and x < T()) {
			x = -x;
			minus = true;
		}

		while (x != T()) {
			T y = x;
			x /= 10;
			y -= x * 10;
			buff[--i] = static_cast<char>('0' + static_cast<int>(y));
		}

		if (minus)
			buff[--i] = '-';

		out_buff_.append(buff.data() + i, buff.end());
		return *this;
	}

	SocketStream& operator<<(const char* s) {
		out_buff_.append(s);
		return *this;
	}

	SocketStream& operator<<(const std::string& s) {
		out_buff_.append(s);
		return *this;
	}

private:
	void writer_main() {
		std::unique_lock<std::mutex> lk(sending_buff_lock);
		// Waiting have to be done at the end of the loop as the first iteration
		// may not do wait() because the data to write may already be there and
		// then the wait() may block indefinitely if it will not be notified
		// a second time
		for (;; start_writing.wait(lk)) {
			if (socket_closed) {
				lk.unlock(); // Do not leave the mutex locked
				return;
			}

			if (not sending_buff.empty())
				for (size_t i = 0; ;) {
					ssize_t rc = write(sock_fd_, sending_buff.data(),
						sending_buff.size());

					// Wait for socket to be writable
					if (rc == 0 or (rc < 0 and errno == EAGAIN)) {
						// Give the main thread the ability to append to the
						// send_buffer without waiting for the socket to become
						// writable
						lk.unlock();

						pollfd pfd = {sock_fd_, POLLOUT, 0};
						if (poll(&pfd, 1, -1) == -1) { // Wait indefinitely
							lk.lock();
							THROW("poll()", errmsg());
						}

						lk.lock();
						continue; // Now we can write
					}

					if (rc < 0) {
						// Error occurred - we cannot write anything
						THROW("write()", errmsg());
						// cerr << "write()" << errmsg() << '\n';
						// sending_buff.clear();
						// break;
					}

					assert(rc >= 0);
					i += rc;

					// Finished writing
					if (i == sending_buff.size()) {
						sending_buff.clear();
						break;
					}

					// Give the main thread a chance to append to the
					// send_buffer in the meantime
					lk.unlock();
					lk.lock();
				}

			finished_writing.notify_one(); // Notifies even if nothing was written
		}
	}

public:
	~SocketStream() {
		// Notify the writer thread to exit
		sending_buff_lock.lock();
		socket_closed = true;
		start_writing.notify_one();
		sending_buff_lock.unlock();

		close(sock_fd_);
		writer_thread_.join();
	}

	// Gives data to the writer thread, so it will be as soon as possible written to the socket
	void flush() {
		{
			std::lock_guard<std::mutex> lk(sending_buff_lock);
			sending_buff += out_buff_;
			start_writing.notify_one();
		}

		out_buff_.resize(0);
	}

	// Ensures that all the data given to the writer thread were written to the
	// socket (if not it waits for the data to be written)
	void sync() {
		std::unique_lock<std::mutex> lk(sending_buff_lock);
		// Tell the writer thread to check if there is more data to read and
		// if not to notify us, the below waiting will occur first because we
		// hold the lock
		start_writing.notify_one();
		// Waits until the writer thread finished writing and notifies us
		finished_writing.wait(lk, [&]{ return sending_buff.empty(); });
		lk.unlock();
	}
};
