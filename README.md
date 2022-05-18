# affine_map
Investigation of iterating certain sets of linear maps on the positive integers, as put forth by an MSE question: https://math.stackexchange.com/questions/4448796/can-we-reach-every-number-of-the-form-8k7-with-those-4-functions 

### The problem

Suppose we start with the number 1, and iteratively apply one of the linear maps 2*x*+1, 3*x*, 3*x*+2, and 3*x*+7. For example, after the first step, we could get 3, 5, or 10. In general we want to know about the behavior of the reachable (and unreachable) integers under this map: What modular congruences do they satisfy? What is their asymptotic density?

This mapping in particular has an unusual chaos to it, but still has sufficient structure to suggest that fairly strong results can be proven about the mapping. However, some of the lesser counterexamples are fairly large—for example, the first reachable number of the form 4*k*+3 is 4443—and so computer analysis is a must. Also, because of the large count of numbers involved, it is an exciting target to practice low-level optimization. 
