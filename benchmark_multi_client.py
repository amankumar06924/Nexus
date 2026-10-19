import socket
import threading
import time
import statistics

HOST = "127.0.0.1"
PORT = 9009

PAYLOAD_SIZE = 64
CLIENT_COUNTS = [1, 2, 4, 8]
DURATION_SECONDS = 10
TIMEOUT = 2

MESSAGE = b"A" * PAYLOAD_SIZE


def receive_exactly(sock, expected_bytes):
    """Read exactly the requested number of bytes."""
    response = bytearray()

    while len(response) < expected_bytes:
        chunk = sock.recv(expected_bytes - len(response))

        if not chunk:
            raise RuntimeError("Connection closed early")

        response.extend(chunk)

    return bytes(response)


def round_trip(sock):
    """Send one message and measure client-observed RTT."""
    start = time.perf_counter_ns()

    sock.sendall(MESSAGE)
    response = receive_exactly(sock, PAYLOAD_SIZE)

    end = time.perf_counter_ns()

    if response != MESSAGE:
        raise RuntimeError("Echo mismatch")

    return (end - start) / 1_000  # microseconds


def percentile(sorted_values, p):
    """Return the nearest-rank percentile."""
    index = max(
        0,
        int((p / 100) * len(sorted_values) + 0.999999) - 1,
    )
    return sorted_values[index]


def client_worker(sock, ready_barrier, start_barrier, control, result):
    """Run requests on one persistent TCP connection."""
    latencies = []
    successes = 0
    errors = 0

    try:
        # Signal that this worker is ready.
        ready_barrier.wait()

        # Wait until the coordinator starts the test.
        start_barrier.wait()

        deadline = control["deadline"]

        while time.perf_counter() < deadline:
            try:
                latency = round_trip(sock)
                latencies.append(latency)
                successes += 1

            except (OSError, RuntimeError):
                errors += 1
                break

    except threading.BrokenBarrierError:
        errors += 1

    finally:
        result["latencies"] = latencies
        result["successes"] = successes
        result["errors"] = errors

        try:
            sock.close()
        except OSError:
            pass


def run_benchmark(client_count):
    """Benchmark the server with a specified number of clients."""
    sockets = []
    threads = []
    results = [{} for _ in range(client_count)]

    print(f"\nPreparing {client_count} client(s)...")

    # Establish all connections before timing starts.
    try:
        for _ in range(client_count):
            sock = socket.create_connection(
                (HOST, PORT),
                timeout=TIMEOUT,
            )
            sock.settimeout(TIMEOUT)
            sockets.append(sock)

    except OSError as error:
        for sock in sockets:
            sock.close()

        print(f"Connection setup failed: {error}")
        return

    # Include the coordinator in each barrier.
    ready_barrier = threading.Barrier(client_count + 1)
    start_barrier = threading.Barrier(client_count + 1)
    control = {"deadline": 0.0}

    for i, sock in enumerate(sockets):
        thread = threading.Thread(
            target=client_worker,
            args=(
                sock,
                ready_barrier,
                start_barrier,
                control,
                results[i],
            ),
        )
        threads.append(thread)
        thread.start()

    try:
        # Wait until every worker is ready.
        ready_barrier.wait(timeout=TIMEOUT + 5)

        # Set the deadline immediately before releasing workers.
        start_time = time.perf_counter()
        control["deadline"] = start_time + DURATION_SECONDS

        # All workers begin the measurement together.
        start_barrier.wait(timeout=TIMEOUT + 5)

    except threading.BrokenBarrierError:
        print("Worker synchronization failed.")

        for sock in sockets:
            try:
                sock.close()
            except OSError:
                pass

        for thread in threads:
            thread.join()

        return

    # Wait for workers to finish their measurements.
    for thread in threads:
        thread.join()

    end_time = time.perf_counter()
    elapsed = end_time - start_time

    # Aggregate results across all clients.
    all_latencies = []
    total_successes = 0
    total_errors = 0

    for result in results:
        all_latencies.extend(result.get("latencies", []))
        total_successes += result.get("successes", 0)
        total_errors += result.get("errors", 0)

    print(f"\n--- {client_count} client(s) ---")
    print(f"Elapsed time:       {elapsed:.3f} seconds")
    print(f"Successful RTTs:    {total_successes:,}")
    print(f"Errors:             {total_errors:,}")

    if not all_latencies:
        print("No successful requests to analyze.")
        return

    all_latencies.sort()

    throughput = total_successes / elapsed
    average_rtt = statistics.mean(all_latencies)
    median_rtt = statistics.median(all_latencies)
    p95_rtt = percentile(all_latencies, 95)
    p99_rtt = percentile(all_latencies, 99)

    print(f"Throughput:         {throughput:,.2f} RTTs/sec")
    print(f"Average RTT:        {average_rtt:.2f} us")
    print(f"P50 RTT:            {median_rtt:.2f} us")
    print(f"P95 RTT:            {p95_rtt:.2f} us")
    print(f"P99 RTT:            {p99_rtt:.2f} us")


def main():
    print("=== Nexus Multi-Client Benchmark ===")
    print(f"Target: {HOST}:{PORT}")
    print(f"Payload: {PAYLOAD_SIZE} bytes")
    print(f"Duration per test: {DURATION_SECONDS}s")
    print(f"Client counts: {CLIENT_COUNTS}")

    for client_count in CLIENT_COUNTS:
        run_benchmark(client_count)

    print("\nBenchmark complete.")


if __name__ == "__main__":
    main()
