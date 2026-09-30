// PDC Lab 03: Processes and Threads
// All four tasks in one program.

using System;
using System.Diagnostics;
using System.Threading;

class Program
{
    static void Main(string[] args)
    {
        string choice = args.Length > 0 ? args[0] : "";

        if (choice == "--child") { Task1_RunAsChild(); return; }
        if (choice == "--noop") { return; }

        if (choice == "")
        {
            Console.WriteLine("PDC Lab 03 - Processes and Threads");
            Console.WriteLine("  1. Task 1: Process Creation and Address-Space Separation");
            Console.WriteLine("  2. Task 2: Summing Array Slices Across Worker Threads");
            Console.WriteLine("  3. Task 3: Measuring Process- vs. Thread-Creation Overhead");
            Console.WriteLine("  4. Task 4: Thread Lifecycle States");
            Console.WriteLine("  5. Run all tasks");
            Console.Write("Select an option (1-5): ");
            choice = Console.ReadLine()?.Trim().Trim('﻿') ?? "";
            Console.WriteLine();
        }

        switch (choice)
        {
            case "1": Task1_RunAsParent(); break;
            case "2": Task2_ArraySum(); break;
            case "3": Task3_CreationOverhead(); break;
            case "4": Task4_ThreadLifecycle(); break;
            case "5":
            case "all":
                Task1_RunAsParent();
                Task2_ArraySum();
                Task3_CreationOverhead();
                Task4_ThreadLifecycle();
                break;
            default:
                Console.WriteLine($"Unknown option '{choice}'. Use 1, 2, 3, 4 or 5.");
                break;
        }
    }

    static string CurrentExePath =>
        Environment.ProcessPath ?? Process.GetCurrentProcess().MainModule!.FileName;

    static void PrintHeader(string title)
    {
        Console.WriteLine();
        Console.WriteLine(new string('=', 70));
        Console.WriteLine(title);
        Console.WriteLine(new string('=', 70));
    }

    // =====================================================================
    // Task 1: Process Creation and Address-Space Separation
    // =====================================================================

    static void Task1_RunAsChild()
    {
        Console.WriteLine($"[Child]  PID = {Environment.ProcessId}");
        int counter = 100;
        counter += 50;
        Console.WriteLine($"[Child]  final counter = {counter}");
    }

    static void Task1_RunAsParent()
    {
        PrintHeader("Task 1: Process Creation and Address-Space Separation");

        Console.WriteLine($"[Parent] PID = {Environment.ProcessId}");
        int counter = 100;
        counter += 1;
        Console.WriteLine($"[Parent] counter after increment = {counter}");

        var startInfo = new ProcessStartInfo
        {
            FileName = CurrentExePath,
            UseShellExecute = false 
        };
        startInfo.ArgumentList.Add("--child");

        Console.WriteLine("[Parent] Launching child process...");
        using (Process child = Process.Start(startInfo)!)
        {
            Console.WriteLine($"[Parent] Started child with PID = {child.Id}");
            child.WaitForExit();
            Console.WriteLine($"[Parent] Child exited with code {child.ExitCode}");
        }

        Console.WriteLine($"[Parent] final counter = {counter}");
        Console.WriteLine("[Parent] Parent and child counters were modified independently (separate address spaces).");
    }

    // =====================================================================
    // Task 2: Summing Array Slices Across Worker Threads
    // =====================================================================

    static long[] data = Array.Empty<long>();
    static long[] partialSums = Array.Empty<long>();
    static int numWorkers;

    static void SumSlice(int idx)
    {
        int sliceSize = data.Length / numWorkers;
        int start = idx * sliceSize;
        int end = (idx == numWorkers - 1) ? data.Length : start + sliceSize;

        long sum = 0;
        for (int i = start; i < end; i++)
        {
            sum += data[i];
        }

        partialSums[idx] = sum;
        Console.WriteLine($"  Worker {idx,2}: [{start,10}, {end,10})  partial sum = {sum}");
    }

    static void Task2_ArraySum()
    {
        PrintHeader("Task 2: Summing Array Slices Across Worker Threads");

        data = new long[10_000_000];
        for (int i = 0; i < data.Length; i++) data[i] = i + 1;

        numWorkers = Environment.ProcessorCount;
        partialSums = new long[numWorkers];

        Console.WriteLine($"Array size:     {data.Length:N0}");
        Console.WriteLine($"Worker threads: {numWorkers}");

        Thread[] threads = new Thread[numWorkers];
        for (int i = 0; i < numWorkers; i++)
        {
            int idx = i; 
            threads[i] = new Thread(() => SumSlice(idx));
            threads[i].Start();
        }

        for (int i = 0; i < numWorkers; i++)
        {
            threads[i].Join();
        }

        long threadedTotal = 0;
        foreach (long partial in partialSums) threadedTotal += partial;

        long sequentialTotal = 0;
        foreach (long value in data) sequentialTotal += value;

        Console.WriteLine();
        Console.WriteLine($"Threaded total:   {threadedTotal}");
        Console.WriteLine($"Sequential total: {sequentialTotal}");
        Console.WriteLine($"Match: {threadedTotal == sequentialTotal}");
    }

    // =====================================================================
    // Task 3: Measuring Process- vs. Thread-Creation Overhead
    // =====================================================================

    const int Iterations = 50;

    static void RunTrivialChildProcess()
    {
        var startInfo = new ProcessStartInfo
        {
            FileName = CurrentExePath,
            UseShellExecute = false
        };
        startInfo.ArgumentList.Add("--noop");

        using Process p = Process.Start(startInfo)!;
        p.WaitForExit();
    }

    static void Task3_CreationOverhead()
    {
        PrintHeader("Task 3: Measuring Process- vs. Thread-Creation Overhead");
        Console.WriteLine($"Iterations: {Iterations}");

        RunTrivialChildProcess();
        new Thread(() => { }).Start();

        var processStopwatch = Stopwatch.StartNew();
        for (int i = 0; i < Iterations; i++)
        {
            RunTrivialChildProcess();
        }
        processStopwatch.Stop();

        var threadStopwatch = Stopwatch.StartNew();
        for (int i = 0; i < Iterations; i++)
        {
            Thread t = new Thread(() => { /* trivial work */ });
            t.Start();
            t.Join();
        }
        threadStopwatch.Stop();

        double avgProcessMs = processStopwatch.Elapsed.TotalMilliseconds / Iterations;
        double avgThreadMs = threadStopwatch.Elapsed.TotalMilliseconds / Iterations;

        Console.WriteLine($"Average process creation time: {avgProcessMs:F3} ms");
        Console.WriteLine($"Average thread creation time:  {avgThreadMs:F3} ms");
        Console.WriteLine($"Process creation was {(avgProcessMs / avgThreadMs):F1}x more expensive than thread creation.");
    }

    // =====================================================================
    // Task 4: Thread Lifecycle States
    // =====================================================================

    static void Worker()
    {
        Thread.Sleep(200);
    }

    static void Task4_ThreadLifecycle()
    {
        PrintHeader("Task 4: Thread Lifecycle States");

        Thread t = new Thread(Worker);
        Console.WriteLine($"After creation:             {t.ThreadState}"); 

        t.Start();
        Console.WriteLine($"Immediately after Start():  {t.ThreadState}");

        Thread.Sleep(50); 
        Console.WriteLine($"While worker is sleeping:   {t.ThreadState}"); 
        t.Join();
        Console.WriteLine($"After Join() completes:     {t.ThreadState}"); 
    }
}
