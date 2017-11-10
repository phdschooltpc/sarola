# Intermittent Thyroid Example using Machine Learning


## Approach 
We took following steps to increase speed per test.
	1.  Removed all mallocs and callocs to have static allocation of memory to variables 
	2.  Divided different function for neural network load and test in tasks so that they are intermittent safe.
	3.  Reduced the number of hidden layers from 5 to 1.
## Tests

We ran 300 tests and the results are as following.

ANN initialisation:
-> execution cycles = 13382
-> execution time = 1.673 ms

Run 300 tests:
-> execution cycles = 11216554 (37388 per test)
-> execution time = 1402.069 ms (4.674 ms per test)

MSE error on 300 test data: 0.020987
