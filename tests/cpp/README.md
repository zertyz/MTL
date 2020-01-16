# MTL Tests

On this folder you'll find the automated tests for the MTL package.

  * ...
  * ....

In addition, there are some spikes useful when developing some modules:


# ReentrantNonBlockingQueueSpikes

This is the helper program used to develop the very first version of `ReentrantNonBlockingQueue.hpp`, before its API got mature enough to be included on this module's unit tests.

Method 1: every save will open up a terminal window, compile and run the program -- useful when developing with just one monitor:

```
export cpp="ReentrantNonBlockingQueueSpikes"; extraWatchList="../../cpp/queue/ReentrantNonBlockingQueue.hpp ../../cpp/time/TimeMeasurements.hpp"; while sleep 1; do for f in ${extraWatchList} ${cpp}.cpp; do if [ "$f" -nt "${cpp}" ]; then xterm -e 'echo "#### `date`: COMPILING..."; g++ -std=c++17 -O3 -march=native -mtune=native -pthread -latomic ${cpp}.cpp -o ${cpp} && echo "#### `date`: RUNNING..." && ./${cpp} || echo -en '\n\n\n\n\n\n'; echo "#### `date`: DONE. Press ENTER to close."; read'; touch "./${cpp}"; fi; done; done
```

Method 2: every save will compile and run the program on an already opened terminal:

```
...
...
```


# FutexAdapterSpikes

This is the helper program used to develop `FutexAdapter.hpp` aiming at controling the execution of threads with the lowest possible latency -- in which case, it will be used as a lock strategy in `FlexLock.hpp`.

Method 1: every save will open up a terminal window, compile and run the program -- useful when developing with just one monitor:

```
export cpp="FutexAdapterSpikes"; extraWatchList="../../cpp/thread/FutexAdapter.hpp ../../cpp/time/TimeMeasurements.hpp"; while sleep 1; do for f in ${extraWatchList} ${cpp}.cpp; do if [ "$f" -nt "${cpp}" ]; then xterm -e 'echo "#### `date`: COMPILING..."; g++ -std=c++17 -O3 -march=native -mtune=native -pthread -latomic -I../../external/EABase/include/Common/ ${cpp}.cpp -o ${cpp} && echo "#### `date`: RUNNING..." && ./${cpp} || echo -en '\n\n\n\n\n\n'; echo "#### `date`: DONE. Press ENTER to close."; read'; touch "./${cpp}"; fi; done; done
```

Method 2: every save will compile and run the program on an already opened terminal:

```
...
...
```


# SpinLockSpikes

This is the helper program used to develop `SpinLock.hpp`, aiming at measuring and stressing the hardware in the following comparisons:
  - producer/consumer latency using a regular mutex;
  - producer/consumer latency using our spin-lock;
  - linear processing -- the same thread 'produces' and 'consumes', without using mutexes or spin-locks;
  
  NOTE: we have two versions of the 'linear processing': one using atomics (so we can measure the thread overhead) and another not using atomics, so we have a baseline for how fast can our hardware do.

Method 1: every save will open up a terminal window, compile and run the program -- useful when developing with just one monitor:

```
export cpp="SpinLockSpikes"; extraWatchList="../../cpp/thread/SpinLock.hpp ../../cpp/thread/FutexAdapter.hpp ../../cpp/time/TimeMeasurements.hpp"; while sleep 1; do for f in ${extraWatchList} ${cpp}.cpp; do if [ "$f" -nt "${cpp}" ]; then xterm -e 'echo "#### `date`: COMPILING..."; g++ -std=c++17 -O3 -march=native -mtune=native -pthread -latomic -I../../external/EABase/include/Common/ ${cpp}.cpp -o ${cpp} && echo "#### `date`: RUNNING..." && ./${cpp} || echo -en '\n\n\n\n\n\n'; echo "#### `date`: DONE. Press ENTER to close."; read'; touch "./${cpp}"; fi; done; done
```

Method 2: every save will compile and run the program on an already opened terminal:

```
...
...
```

for code in FutexAdapterSpikes.cpp ReentrantNonBlockingQueueSpikes.cpp SpinLockSpikes.cpp UnorderedArrayBasedReentrantStackSpikes.cpp CppUtilsSpikes.cpp; do for compiler in g++ clang++; do echo -en "`date`: Compiling $code with $compiler..."; $compiler -std=c++17 -O3 -march=native -mcpu=native -mtune=native -mfloat-abi=hard -mfpu=vfp -I../../external/EABase/include/Common/ -pthread -latomic $code -o ${code}.$compiler && echo " OK"; done; done

