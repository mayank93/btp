===============================================

Iterative Clustering - Codebase

===============================================

Mayank Gupta (mayank.g@students.iiit.ac.in)
Ankush Jain (ankush.jain@students.iiit.ac.in)

-----------------------------------------------

Four datasets have been readied for the code - irisData, zooData, seedData, waterData. A folder corresponding to each of the datasets is there.

Each folder has the following three files - 

	* categories
		- Contains the concept hierarchy
	* data.txt
		- Contains the properly formatted data
	* nat	
		- Contains the natural clusters for the dataset

To run the evaluation suite on any dataset, run the following command

	- ./test.sh [irisData|zooData|seedData|waterData] [0|1]

		* The first parameter is the dataset to run the tests on
		* The second parameter is the flag to toggle merging after cluster generation
			0 means don't merge, 1 means merge

	Any parameters that need to be changed (like beta) can be changed in the file test.sh
