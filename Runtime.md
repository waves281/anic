The following is a rough outline of the ANI runtime system and how ANI programs are actually compiled/executed. It is rather technical and assumes at least a moderate understanding of ANI language concepts; it's reccommended that you read through the [Tutorial](Tutorial.md) first. That said, let's begin.

ANI programs are made up of pipes, but ANI program execution works on even finer-grained units called pipe chunks, which are pieces of pipes. Essentially, anything between `{}` curly braces, `;` semicolons, or non-primitive filter invocations counts as a pipe chunk.

The following is a single pipe chunk (it's also a single pipe):

```
1 + 2 ->std.out;
```

The following consists of two pipe chunks (it's also two pipes):

```
x = [[int\]] <- 1;
\x + 3 ->std.out;
```

So far, this is all intuitive. However, here's a single pipe consisting of _two_ pipe chunks:

```
2 * 3 [[int\ x]] {\x + 5 ->std.out};
```

The pipe is fractured into two chunks at the fork through the anonymous object with header `[[int\ x]] {` (as mentioned before, curly brackets and semicolons divide pipes into chunks).

As one final example, here's another case of a single top-level pipe, but this time containing three pipe chunks. As an exercise, you might want to try parsing out the pipe chunks in your head.

```
"Hello, World!" {
	s = [[string\]] <- (\.. + " Hello, again!");
	\s ->std.out;
};
```

The purpose of making this pipe/pipe chunk distinction is that as far as the runtime is concerned, _it's not the pipes that get executed; it's the pipe chunks_. Thus, our granularity of execution is a single pipe chunk. This works very well, since with ANI's dataflow paradigm, although pipe chunks are naturally interlinked with each other (often directly, as in the above example), they are otherwise independent units of execution and can be executed in parallel. This is the precisely the implicit fine-grained parallelism that the compiler and runtime exploit to give ANI program execution its massively parallel nature.

Now that we understand the notion of a pipe chunk, we can understand ANI thread dispatch, which is really quite simple: execution threads are dynamically dispatched to execute pipe chunks. Of course, this requires elaboration.

When an ANI program starts up, it branches into N threads of execution, where N is at least 2, but otherwise determined by the compiler and usually equal to the number of processing elements (processor cores) available on the target machine. (N-1) of these are deemed worker threads, and one is deemed an administrator thread whose job it is to shepherd, dispatch, and route the worker threads appropriately. How exactly is this done?

Each worker thread is given its own dispatch buffer which contains a linear list of the addresses to the pipe chunks that it is to execute. The thread's personal stack pointer always points into the next entry in its dispatch buffer. Combined with the fact that at the end of every pipe chunk is a `ret` (function return) instruction, this means that threads linearly go through their dispatch buffers, gradually popping pipe chunk addresses off of the stack and executing the chunks in the order that they are listed. The buffer is bounded and circular so that execution never runs off the end of the buffer; it simply loops back to the top.

So now the question arises of how these dispatch buffers are filled. Naturally, this is the administrator thread's job; it monitors the worker thread's progress during execution and optimistically dumps new pipe chunk addresses into the buffers while the program runs. The hope is that if the administrator is doing its job fast enough, then by the time a thread is done with one pipe chunk, it immediately jumps to the next one that needs to be executed without ever needing to stop to figure out what to do next.

The dataflow paradigm as used in ANI lends itself very well to a value-stack model of pipe chunk execution, and the optimal place to put intermediate stack values is into processor registers. Since pipe chunks are extremely small and fine-grained, this means that most of the time, we can keep intermediate values in registers alone. This works extremely well, since in a great deal of cases, pipe chunk dependencies mean that we can jump from one chunk to the next directly without needing to do anything special to context-switch between pipe chunks; one chunk's output stack becomes the next one's input stack, and the routed values are carried over implicitly using processor registers. The arbitrator thread attempts to make routing decisions in such a way that this is the case as often as possible.

Of course, this is not always doable, and in some cases (such as forks), we will necessarily need to dump and/or load context, since the same value stack sometimes needs to be reused as input into multiple pipe chunks. Wherever this is necessary, the administrator inserts into the dispatch buffer addresses to special compiler generated context-dumping and context-loading pipe chunks that dump/load stack values to/from memory as necessary to maintain the proper state of data.

One question that we have not addressed yet is that of what happens if the administrator happens to not be quick enough with filling dispatch buffers and a thread runs off of the end of its corresponding buffer. The simple solution is that dispatch buffers are initialized to contain nothing but a list of pointers to a default "dispatch overrun" function, which will be jumped to whenever the administrator fails to be quick enough with laying out the metaphorical carpet ahead of any given thread. This function simply busy-loops until the administrator gets its act together and re-fills the dispatch buffer a certain amount (so that we don't immediately overrun again after one chunk of execution), at which point the next address is popped off the dispatch buffer and execution continues.

All of this keeps happening until the administrator realizes that there are no more pipe chunks to schedule (the program has reached a stable state in which nothing further is going to be triggered). At this point, the administrator simply waits for all threads to finish what they're doing and overrun their dispatch buffers, and then it halts the program.

_To be continued..._