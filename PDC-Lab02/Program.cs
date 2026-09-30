using System;
using System.Threading;

class Program
{
    // Worker method: runs on its own thread.
    // Receives its thread index (boxed as object) as 'arg'.
    static void Worker(object? arg)
    {
        long id = (long)arg!;
        Console.WriteLine($"Thread {id}: starting");

        // Optional (5.5): report the logical CPU this thread is actually
        // executing on at this moment. The OS scheduler can move it later,
        // so this is just a snapshot.
        int cpu = Thread.GetCurrentProcessorId();
        Console.WriteLine($"Thread {id}: running on logical CPU {cpu}");

        Console.WriteLine($"Thread {id}: finished");
    }

    static void Main()
    {
        // Step 1: detect the number of logical cores available to this process.
        int numCores = Environment.ProcessorCount;
        Console.WriteLine($"Detected logical cores: {numCores}");

        // Step 2: dynamically allocate the thread array — size comes from
        // numCores at runtime, never hard-coded.
        Thread[] threads = new Thread[numCores];

        // Step 3: create exactly one thread per detected core.
        for (int i = 0; i < numCores; i++)
        {
            long idx = i; // capture a local copy so each thread gets its own index
            threads[i] = new Thread(Worker);
            threads[i].Start(idx);
        }

        // Step 4: join every thread before continuing — blocks Main until
        // each worker has completed.
        for (int i = 0; i < numCores; i++)
        {
            threads[i].Join();
        }

        // Step 5: confirm how many threads were actually created and joined.
        Console.WriteLine($"All {threads.Length} threads completed.");
    }
}