# Introduction #

<img src='http://lh5.ggpht.com/_rjzuXZbgzw0/S07KNoh4r_I/AAAAAAAAAB0/UmOFpej7jTs/velociraptor3.jpg' align='right' width='33%'> <b>anic</b> is the reference implementation compiler for the experimental, high-performance, implicitly parallel, deadlock-free general-purpose dataflow programming language <b>ANI</b>.<br><br>Portably written using the GNU toolchain, <i>anic</i> works on all of the popular operating systems, including <code>*</code>nix, Mac OS X, and Windows (via Cygwin).<br><br>The project is free and open; you can get a copy of the <i>anic</i> source anytime:<br><br><table><thead><th> <code>hg clone https://anic.googlecode.com/hg/ anic</code> </th></thead><tbody></tbody></table>

<h3>The Fast Track</h3>

Want to get started with <i>ANI</i> programming right away? Head over straight to the <a href='http://code.google.com/p/anic/wiki/Tutorial'>ANI Tutorial</a>.<br>
<br>
Got burning philosophical questions? The <a href='http://code.google.com/p/anic/wiki/FAQ'>FAQ is this way</a>.<br>
<br>
Have something to say? Join in on the <a href='http://groups.google.com/group/ani-compiler'>official discussion group</a>.<br>
<br>
Want to know more about the project? Read on.<br>
<br>
<h2>The Quirks</h2>

<i>ANI</i> is probably unlike any programming language you've encountered; it does away with state, variables, commands, stacks, and memory itself, as we know it. In <i>ANI</i>, the compiler sequences the ordering of your program logic for you, laying out as much of it as possible in parallel, and guaranteeing that the resulting binary will be statically safe and deadlock-free. Best of all, compiler technology has advanced to the point where you don't need to understand any of this to leverage it; that's <i>anic</i>'s job!<br>
<br>
Crazy? Most definitely. And yet strangely enough, it works!<br>
<br>
<h2>Hello, World!</h2>

The language couldn't possibly be simpler...<br>
<br>
<pre><code>"Hello, World!" -&gt;std.out<br>
</code></pre>

<h2>Dining Philosophers Problem - A Complete, Well-Written, Correct Program</h2>

...or any more powerful...<br>
<br>
<pre><code>philosopher = []{<br>
	id = [int\];<br>
	chopstick = [int\];<br>
	nextPhil = [philosopher\];<br>
	=;<br>
	=[int newId] { [\newId] &lt;-&gt;id; }<br>
<br>
	getChopsticks = [--&gt; ?] { \chopstick, \nextPhil.chopstick --&gt; };<br>
	returnChopsticks = [int\ cs1, int\ cs2] { \cs1 -&gt;chopstick; \cs2 -&gt;nextPhil.chopstick; };<br>
	eat = [int\ cs1, int\ cs2 --&gt; ?] {<br>
		"Philosopher " + id + " eating...\n" -&gt;std.out;<br>
		\cs1, \cs2 --&gt;;<br>
	};<br>
	{ std.randInt std.delay getChopsticks eat returnChopsticks &lt;- };<br>
};<br>
<br>
numPhils = 5;<br>
<br>
philPool = [philosopher[numPhils]];<br>
numPhils std.gen &lt;| [int curId] {<br>
	curId -&gt;philPool.[curId];<br>
	\philPool.[(curId + 1) % numPhils] -&gt;philPool.[curId].nextPhil;<br>
};<br>
</code></pre>

<i>(for a more detailed explanation of why this works, see the <a href='http://code.google.com/p/anic/wiki/FAQ'>FAQ</a>)</i>

Compare this with Wikipedia's <a href='http://en.wikipedia.org/wiki/Dining_philosophers_problem#Example_Solution'>much longer, much less efficient, and unintuitive Pascal solution to the problem</a> -- and that's actually a "<i>simple</i>" solution leaning on high-level monitor constructions. For the <i>real</i> nightmare, try implementing this thing using pthreads (the industry standard). Given half an hour and some frustration, a well-experienced programmer could probably do it.<br>
<br>
But why? There's <i>ANI</i>.<br>
<br>
<h2>The Aim</h2>

Try to imagine, if you will, the amount of time and effort it would take you to write a bug-free, efficiently multithreaded real-time clock + infix calculator hybrid application in a language like C.<br>
<br>
While you're thinking, here's a compacted but 100% complete implementation in <i>ANI</i> -- and this isn't even leveraging any libraries!<br>
<br>
<pre><code>a=[int\]&lt;-0; op=[char\]&lt;-' '; b=[int\]&lt;-0; r=[int\]&lt;-0;<br>
0 { clock =&gt; [int ms] { ("\r" + ms/1000.0 + ":" + a + op + b + "=" + r) -&gt;std.out; 1 std.delay (ms+1) clock} };<br>
inLoop =&gt; {\std.in-&gt;a \std.in-&gt;op \std.in-&gt;b inLoop};<br>
\\op ?? {'+': (\a+\b) '-': (\a-\b) '*': (\a*\b) '/': (\a/\b) : 0} &lt;-&gt;r;<br>
</code></pre>

<i>ANI</i> is an attempt to fuse the intuitive feel of shell scripting (and all of its perks like implicit parallelism) with the safety of strict compilation and the speed of hand-optimized parallel assembly: in other words, lightweight programming that runs even faster than typical C.<br>
<br>
In short, <i>ANI</i> seeks to break out of the shackles of imperative programming -- a stale paradigm which for four decades has produced hundreds of clones of the same fundamental feature set, none of which offer <i>intuitive</i> hands-off concurrency, and differing only in what lengths they go to to sugar-coat the embarrassing truth that they're all just increasingly high-level assemblers at heart; <i>ANI</i> is inspired by the realization that in today's programming environment, your compiler should be doing more for you than a blind language translation!<br>
<br>
<h2>The Bottom Line</h2>

Think of <i>ANI</i> as a way to write fast, statically-guaranteed safe, well-structured, and highly parallelized software without ever encountering memory management, threads, locks, semaphores, critical sections, race conditions, or deadlock.<br>
<br>
The central philosophy of <i>ANI</i> programming is that you "<a href='http://en.wikipedia.org/wiki/Fire-and-forget'>type-and-forget</a>". You describe what you want to happen to your data, and it just gets done -- and fast. <i>ANI</i> is lightweight like a shell script but fast like C, safe like Java, and implicitly massively parallel like a language for the parallel processing age should be.<br>
<br>
<i>ANI</i> accomplishes these ambitious goals by way of two novel approaches:<br>
<br>
<ul><li>a paradigm shift away from the intractable chaos of imperative-style memory twiddling in favor of structured but flexible dataflow pipelines that can be heavily optimized through static analysis, and<br>
</li><li>a paper-thin but extremely powerful micro-scheduling runtime that exploits experimental ideas such as dynamic code polymorphism to deliver fine-grained, safe, and fully implicit parallelism from the compiled pipelines</li></ul>

<h3>Warning: Computer Science Content!</h3>

To those more technically inclined, <i>anic</i> compiles source-specified pipeline definitions down to object code modules, which are linked with a runtime providing initialization code and a root arbitrator thread; the arbitrator spawns worker threads which are dynamically dispatched to the compiled pipelines in such a way that there are no memory conflicts.<br>
<br>
Think of <i>ANI</i> source code as a blueprint for a set of train tracks. <i>anic</i> looks at this and builds a real train track for you (making it better wherever it can). The program is run by putting running trains onto the tracks, and it turns out that <i>anic</i> also hired a system administrator for you who will keep an eye on the trains to make sure they don't crash. That's <i>ANI</i> in a technical nutshell!<br>
<br>
<hr />

<h1>Tutorial</h1>

Where can you get started with <i>ANI</i>? Right here! An introductory tutorial is available on the <a href='http://code.google.com/p/anic/wiki/Tutorial'>project wiki</a>.<br>
<br>
<hr />

<h1>Discussion Group</h1>

For those wanting to keep up to date on ANI/anic-related issues, a <a href='http://groups.google.com/group/ani-compiler'>discussion group</a> is available. Even if all you have to offer is criticism, the project could definitely use the help!<br>
<br>
<hr />

<h1>Status</h1>

The project is currently in alpha development, and we're looking for help to reach that all-important <i>1.0</i> milestone; every bit makes things go quicker. Those insterested are encouraged to join the <a href='http://groups.google.com/group/ani-compiler'>official discussion group</a> and see how they can be part of shaping an exciting new way of programming.