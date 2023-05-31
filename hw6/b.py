import numpy as np

A = np.array([[0,0,1,0,0,0,0],
[0,1,1,0,0,0,0],
[1,0,1,2,0,0,0],
[0,0,0,1,1,0,0],
[0,0,0,0,0,0,1],
[0,0,0,0,0,1,1],
[0,0,0,2,1,0,1]])
B = np.transpose(A)
C =np.dot(B,A)
print(C)
eig_val, eig_vec = np.linalg.eig(C)
print('Eigenvalues:')
print(eig_val)
print()
print('Eigenvectors:')
print(eig_vec)