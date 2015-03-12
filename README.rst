anic
====

This project is dead. I'm importing it to GitHub from the Google Code repo since Google Code is also `dead <http://google-opensource.blogspot.com/2015/03/farewell-to-google-code.html>`_, and I didn't want this project to disappear forever. The ideas it presents are too valuable to go to waste.

So...here's the entire repo, thanks to the `Export to GitHub` button in Google Code.

TODO: move the wiki over

Here's the original home page, re-typed in rST:

Introduction
************

.. image:: https://github.com/kirbyfan64/anic/blob/master/ani.jpg
   :target: https://github.com/kirbyfan64/anic/blob/master

**anic** is the reference implementation compiler for the experimental, high-performance, implicitly parallel, deadlock-free general-purpose dataflow programming language **ANI**.

Portably written using the GNU toolchain, *anic* works on all of the popular operating systems, including \*nix, Mac OS X, and Windows (via Cygwin).

The project is free and open; you can get a copy of the *anic* source anytime::

    hg clone https://anic.googlecode.com/hg/ anic

The Fast Track
--------------

Want to get started with *ANI* programming right away? Head over straight to the ANI Tutorial.

Got burning philosophical questions? The FAQ is this way.

Have something to say? Join in on the `official discussion group <http://groups.google.com/group/ani-compiler>`_.

Want to know more about the project? Read on.

The Quirks
----------

*ANI* is probably unlike any programming language you've encountered; it does away with state, variables, commands, stacks, and memory itself, as we know it. In *ANI*, the compiler sequences the ordering of your program logic for you, laying out as much of it as possible in parallel, and guaranteeing that the resulting binary will be statically safe and deadlock-free. Best of all, compiler technology has advanced to the point where you don't need to understand any of this to leverage it; that's *anic*'s job!

Crazy? Most definitely. And yet strangely enough, it works!

Hello, World!
-------------

The language couldn't possibly be simpler... ::
   
   "Hello, World!" ->std.out

Dining Philosophers Problem - A Complete, Well-Written, Correct Program
-----------------------------------------------------------------------

...or any more powerful... ::
   
   philosopher = []{
           id = [int\];
           chopstick = [int\];
           nextPhil = [philosopher\];
           =;
           =[int newId] { [\newId] <->id; }
   
           getChopsticks = [--> ?] { \chopstick, \nextPhil.chopstick --> };
           returnChopsticks = [int\ cs1, int\ cs2] { \cs1 ->chopstick; \cs2 ->nextPhil.chopstick; };
           eat = [int\ cs1, int\ cs2 --> ?] {
                   "Philosopher " + id + " eating...\n" ->std.out;
                   \cs1, \cs2 -->;
           };
           { std.randInt std.delay getChopsticks eat returnChopsticks <- };
   };
   
   numPhils = 5;
   
   philPool = [philosopher[numPhils]];
   numPhils std.gen <| [int curId] {
           curId ->philPool.[curId];
           \philPool.[(curId + 1) % numPhils] ->philPool.[curId].nextPhil;
   };

*(for a more detailed explanation of why this works, see the FAQ)*

Compare this with Wikipedia's `much longer, much less efficient, and unintuitive Pascal solution to the problem <http://en.wikipedia.org/wiki/Dining_philosophers_problem#Example_Solution>`_ -- and that's actually a "*simple*" solution leaning on high-level monitor constructions. For the *real* nightmare, try implementing this thing using pthreads (the industry standard). Given half an hour and some frustration, a well-experienced programmer could probably do it.

But why? There's *ANI*.

The Aim
-------

Try to imagine, if you will, the amount of time and effort it would take you to write a bug-free, efficiently multithreaded real-time clock + infix calculator hybrid application in a language like C.

While you're thinking, here's a compacted but 100% complete implementation in *ANI* -- and this isn't even leveraging any libraries! ::
   
   a=[int\]<-0; op=[char\]<-' '; b=[int\]<-0; r=[int\]<-0;
   0 { clock => [int ms] { ("\r" + ms/1000.0 + ":" + a + op + b + "=" + r) ->std.out; 1 std.delay (ms+1) clock} };
   inLoop => {\std.in->a \std.in->op \std.in->b inLoop};
   \\op ?? {'+': (\a+\b) '-': (\a-\b) '*': (\a*\b) '/': (\a/\b) : 0} <->r;

*ANI* is an attempt to fuse the intuitive feel of shell scripting (and all of its perks like implicit parallelism) with the safety of strict compilation and the speed of hand-optimized parallel assembly: in other words, lightweight programming that runs even faster than typical C.

In short, *ANI* seeks to break out of the shackles of imperative programming -- a stale paradigm which for four decades has produced hundreds of clones of the same fundamental feature set, none of which offer *intuitive* hands-off concurrency, and differing only in what lengths they go to to sugar-coat the embarrassing truth that they're all just increasingly high-level assemblers at heart; *ANI* is inspired by the realization that in today's programming environment, your compiler should be doing more for you than a blind language translation!

The Bottom Line
---------------

Think of *ANI* as a way to write fast, statically-guaranteed safe, well-structured, and highly parallelized software without ever encountering memory management, threads, locks, semaphores, critical sections, race conditions, or deadlock.

The central philosophy of *ANI* programming is that you "`type-and-forget <http://en.wikipedia.org/wiki/Fire-and-forget>`_". You describe what you want to happen to your data, and it just gets done -- and fast. *ANI* is lightweight like a shell script but fast like C, safe like Java, and implicitly massively parallel like a language for the parallel processing age should be.

*ANI* accomplishes these ambitious goals by way of two novel approaches:

- a paradigm shift away from the intractable chaos of imperative-style memory twiddling in favor of structured but flexible dataflow pipelines that can be heavily optimized through static analysis, and
- a paper-thin but extremely powerful micro-scheduling runtime that exploits experimental ideas such as dynamic code polymorphism to deliver fine-grained, safe, and fully implicit parallelism from the compiled pipelines

Warning: Computer Science Content!
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To those more technically inclined, *anic* compiles source-specified pipeline definitions down to object code modules, which are linked with a runtime providing initialization code and a root arbitrator thread; the arbitrator spawns worker threads which are dynamically dispatched to the compiled pipelines in such a way that there are no memory conflicts.

Think of *ANI* source code as a blueprint for a set of train tracks. *anic* looks at this and builds a real train track for you (making it better wherever it can). The program is run by putting running trains onto the tracks, and it turns out that *anic* also hired a system administrator for you who will keep an eye on the trains to make sure they don't crash. That's *ANI* in a technical nutshell!

Tutorial
********

Where can you get started with *ANI*? Right here! An introductory tutorial is available on the project wiki.

Discussion Group
****************

For those wanting to keep up to date on ANI/anic-related issues, a discussion group is available. Even if all you have to offer is criticism, the project could definitely use the help!

Status
******

The project is currently in alpha development, and we're looking for help to reach that all-important *1.0* milestone; every bit makes things go quicker. Those insterested are encouraged to join the official discussion group and see how they can be part of shaping an exciting new way of programming.

