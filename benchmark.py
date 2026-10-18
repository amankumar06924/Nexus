
import socket
import time
import statistics

HOST = "127.0.0.1"
PORT = 9009

PAYLOAD_SIZE = 64
WARMUP_ROUNDS = 1_000
MEASURED_ROUNDS = 10_000
TIMEOUT = 5

MESSAGE = b"A" * PAYLOAD_SIZE


def receive_exactly(sock, expected_bytes):
    """Read until exactly expected_bytes have arrived."""
    response = bytearray()

    while len(response) < expected_bytes:
        chunk = sock.recv(expected_bytes - len(response))
        if not chunk:
            raise RuntimeError(
                f"Connection closed early: received "
                f"{len(response)} of {expected_bytes} bytes"
            )
        response.extend(chunk)
    return bytes(response)

def round_trip(sock):
    """Send one message and return its client-observed RTT in microseconds."""
    start = time.perf_counter_ns()

    sock.sendall(MESSAGE)
    response = receive_exactly(sock, PAYLOAD_SIZE)

    end = time.perf_counter_ns()

    if response != MESSAGE:
        raise RuntimeError("Echo mismatch: received data differs from sent data")

    return (end - start) / 1_000

def percentile(sorted_values, percentile):
    """Calculate a percentile using the nearest-rank method."""
    index = max(0, int((percentile / 100) * len(sorted_values) + 0.999999) - 1)
    return sorted_values[index]

def main():
    print("=== Nexus RTT Benchmark ===")
    print(f"Target:          {HOST}:{PORT}")
    print(f"Payload:         {PAYLOAD_SIZE} bytes")
    print(f"Warm-up rounds:  {WARMUP_ROUNDS:,}")
    print(f"Measured rounds: {MEASURED_ROUNDS:,}")
    print()
    try:
        with socket.create_connection((HOST, PORT), timeout=TIMEOUT) as sock:
            sock.settimeout(TIMEOUT)
            print("Running warm-up...")
            for _ in range(WARMUP_ROUNDS):
                round_trip(sock)
            print("Running measured benchmark...")
            latencies_us = []
            benchmark_start = time.perf_counter_ns()
            for i in range(MEASURED_ROUNDS):
                latency = round_trip(sock)
                latencies_us.append(latency)

                if (i + 1) % 2_000 == 0:
                    print(f"Progress: {i + 1:,}/{MEASURED_ROUNDS:,}")

            benchmark_end = time.perf_counter_ns()
    except (OSError, RuntimeError) as error:
        print(f"\nBenchmark failed: {error}")
        return

    sorted_latencies = sorted(latencies_us)
    total_seconds = (benchmark_end - benchmark_start) / 1_000_000_000
    average_us = statistics.mean(latencies_us)
    median_us = statistics.median(latencies_us)
    p95_us = percentile(sorted_latencies, 95)
    p99_us = percentile(sorted_latencies, 99)
    rps = MEASURED_ROUNDS / total_seconds

    print("\n=== Results ===")
    print(f"Successful requests: {len(latencies_us):,}")
    print(f"Payload per request: {PAYLOAD_SIZE} bytes")
    print(f"Total measured time:  {total_seconds:.3f} seconds")
    print(f"Average RTT:          {average_us:.2f} us")
    print(f"P50 RTT:              {median_us:.2f} us")
    print(f"P95 RTT:              {p95_us:.2f} us")
    print(f"P99 RTT:              {p99_us:.2f} us")
    print(f"Request-response rate:{rps:10,.2f} round trips/sec")


if __name__ == "__main__":
    main()
