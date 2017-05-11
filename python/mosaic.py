def mosaic(img):
    """
    Create a 2-D mosaic of images from an n-D image. An attempt is made to 
    make the resulting 2-D image as square as possible.

    Parameters
    ----------
    img : ndarray
        n-dimensional image be tiled into a mosaic

    Returns
    -------
    mosaic : 2-d image
        Tiled mosaic of images.
    """
    from numpy import ix_, array, zeros, arange
    from math import sqrt, floor
    if len(img.shape) <= 2:   # already 2-D, so skip the rest
        return img
    img = array(img)
    nr, nc = img.shape[-2:]    # take off last two dimensions, rest are lumped.
    img = img.reshape((-1, nr, nc))
    nz = img.shape[0]
    n = int(floor(sqrt(nz)))  # starting guess for tiling dimensions
    # find largest integer less than or equal to sqrt that evenly divides the
    # number of 2-d images in the n-d image.
    m = [x for x in xrange(1, n+1) if nz % x == 0]
    m = m[-1]
    j = nz / m      # figure out the most square dimensions
    n2 = min(j, m)
    n1 = max(j, m)
    M = zeros((nr*n2, nc*n1), dtype=img.dtype)
    for j2 in range(n2):      # stick them together
        for j1 in range(n1):  # there is probably a better way to do this
            rows = nr*j2 + arange(nr)
            cols = nc*j1 + arange(nc)
            M[ix_(rows, cols)] = img[j1+j2*n1, :, :]
    return M


def main():
    import scipy.misc
    import matplotlib.pyplot as pl
    import numpy as np
    img = scipy.misc.lena()
    nz = 64 
    n = img.shape[0]
    img = np.tile(img, [nz, 1, 1])
    pl.imshow(mosaic(img), cmap='gray')
    pl.show()


if __name__ == '__main__':
    main()

