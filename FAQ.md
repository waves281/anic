**Q: How do I pronounce ANI / anic? What does ANI stand for?**

**A**: _ANI is pronounced just like the feminine name "Annie"; anic is just "panic" without the 'p'._

ANI stands for "[Animus](http://en.wikipedia.org/wiki/Anima_and_animus)", but apart from the animus being a neat concept, the name was chosen because it generates reasonably clean search hits, in an attempt to avoid the name stomping fiasco that Google instigated with Golang.

**Q: Faster than C? How is that possible?**

**A**: _Traditional programs are single-threaded; in most cases, this is the category that C programs fall into._

In that sense, however, these programs are limited in the resources they can leverage; they can't take advantage of parallel execution on multiprocessor architectures, the kind that we're seeing become ever more prevalent today (and this trend looks like it's here to stay). Of course, if you're comfortable with juggling chainsaws, you can write multithreaded programs in C, but ask anyone who's worked on a large multithreaded system written in C and you'll see them cringe. C is an inherently single-threaded language with ugly multithreading support tacked on once the goof of originally omitting it was obvious. That means ugly code and cluttered binaries that will never run as fast as they could in a language designed from the ground up to be based around parallel execution.

That's the kind of language ANI is; in fact, ANI is so parallel that it's actually difficult to write traditional sequential programs in it. But the point is you should never have to: single-threaded programs are a thing of the past. The future lies in concurrency.

**Q: Why are backslashes (`\`) used as language operators? Isn't that confusing, given they're used in other languages as escape characters?**

**A**: _This is a valid point, but backslashes were chosen for a purely pragmatic reason; on virutally all keyboards, backslashes are very easy to type (requiring only a single keystroke). This is a handy property for backslashes to have because in ANI, you'll be typing them a lot!_

Incomers from other languages might be thrown off a tiny bit, but a programmer that's spent some time with ANI will quickly come to realize that there is actually _never_ any good reason to end a line of ANI code with a syntactual backslash! If one insists on doing so anyway, they are writing ill-formatted code that would be confusing regardless of how backslashes are interpreted by the language. Thus, the backslash conflict is there in theory but irrelevant in practice.

The usage of `\` in the language syntax is a thought-out practical compromise, though the issue may be reconsidered in the future depending on programmer feedback.

**Q: How does one implement classical parallel algorithm X in ANI?**

**A**: _Such questions are probably misguided._

While nearly all instances of X are classic "embarassingly parallel" algorithms in the imperative programming sense, ANI's implicit fine-grained dataflow parallelism lends itself to a much, much broader field of application than just the explicitly parallel stuff most programmers are brought up with. In fact, ANI is designed to abstract away from the idea of an "algorithm" altogether, which further goes to demonstrate that you can't neatly apply explicit ideas of parallelism to an implicitly parallel language.

ANI aims to extract "good" parallelism (in the sense that with optimization, it converges to optimality) out of _nearly anything_ just by the way that the dataflow paradigm forces you to think. Of course, this means that the naive parallelism of the classic "embarassingly parallel" problems naturally gets exploited -- but not only that, the language and compiler are specifically designed to discover and exploit the potential parallelism you didn't even realize was there.

So although it wouldn't be hard to re-implement the classic parallel algorithms in ANI, to do so for the sake of doing so would be to miss the point of the language. The reason those problems are considered classic is because they've been solved to death, and unlike every imperative language rehash that's come about in the last 20 years, ANI isn't designed to redo what's already been done. ANI is an experiment in thinking outside of the box and an attempt at altogether breaking free of the explicit imperative programming mindset (parallel or not).

One of ANI's central goals is to help programmers not have to think about the specifics of parallel algorithms in the first place by shifting the burden of discovering parallelism to the compiler. Thus, to ask how one would implement specific parallel algorithm X in ANI is probably the wrong kind of question to be asking, and an undesirable step backwards for a forward-thinking language.

**Q: How does ANI resolve the fundamental issue of deadlock in parallel programming? How does the Dining Philosophers example on the main page work?**

**A**: _In ANI, under the hood, pipes implicitly enforce total resource orderings for acquiring data. Somewhat surprisingly, deadlock theory guarantees that there will be no deadlock if this condition is met._

The compiler attempts to figure out a static ordering for most data acquisition, but this is not possible with array elements; thus, array elements are resource-ordered using metadata annotations at runtime. Where the compiler cannot come up with a total resource ordering for data, and it comes across a dependency that it knows it won't be able to fully resolve with run-time ordering annotations, it will reject the program as unsafe, pointing out the dependency chain what causes the problem. Since this is dataflow programming, such analyses are very natural to do.

In the case of the Dining Philosophers program, a runtime resource ordering will suffice to ensure each philosopher (even the last one) will pick up the chopstick on their left first; the runtime will in this case allow for the chopstick acquisition order to be dynamically swapped as necessary.

Of course, this is rather technical and the point is that in most cases, programmers don't need to care about these details -- that's the compiler's burden! The one case where programmers will see them is when their program is provably unsafe and the compiler can't figure out a sensible way to fix the issue. Such a situation is at best an indication of a highly confusing flow of data, and at worst a nasty parallel programming bug; in both cases, the compiler would do well to flag an error, and that's exactly what it does.

**Q: What's the story behind the logo?**

**A**: _anic's logo is a [velociraptor](http://en.wikipedia.org/wiki/Velociraptor)._

In a lot of ways, _ANI_ is a new take on some age-old but little used ideas. Like the velociraptor, dataflow programming has been around for a long time (arguably since the first processor), but few notable examples have come to the surface. Just like the scientists who recently concluded that velociraptors were indeed feathered creatures, the ANI project is founded on the presumption that dataflow programming has long been misunderstood but has wings we never knew were there.

The image used for the logo is based on an illustration by Matt Martyniuk. See [the image on the Wikipedia Commons](http://commons.wikimedia.org/wiki/File:Velociraptor_dinoguy2.jpg).

_To be continued..._