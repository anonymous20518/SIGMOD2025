# Experiment Datasets

_Populate this directory with the following datasets in order to reproduce experiments from paper submission #840_

## List of Datasets

 * [Amazon](http://data.law.di.unimi.it/webdata/amazon-2008/amazon-2008.graph)
 * [CitPatent](https://snap.stanford.edu/data/cit-Patents.html)
 * [DBLP](https://snap.stanford.edu/data/bigdata/communities/com-dblp.ungraph.txt.gz)
 * [LiveJournal](https://snap.stanford.edu/data/soc-LiveJournal1.html)
 * [WikiTalk](https://snap.stanford.edu/data/wiki-Talk.html)
 * [Youtube](https://snap.stanford.edu/data/bigdata/communities/com-youtube.ungraph.txt.gz)

## Graph G=(V,E) Preparation Instructions

1. Prep the edge list files to only include edges by excluding all header/metadata lines and save each as a .txt file.
2. Pass the .txt file to the `convertToUndirected()` function in preprocessing.ipynb notebook to pre-process the file (make edges undirected and remove self loops). 

## Vertex Label Preparation Instructions

Use the `make*Labels()` functions in the preprocessing.ipynb notebook with the desired number of nodes and dimension (use n=6009555 and d = 2, 3, 4, 5 to be able to reproduce experiment results).
