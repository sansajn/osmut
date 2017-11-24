# downloads osm tiles (for python 2.7)
import urllib2, os, sys

DEFAULT_MIN_DEPTH=0
DEFAULT_MAX_DEPTH=9

def main(args):
	if len(args) > 2:
		min_depth = int(args[2])
	else:
		min_depth = DEFAULT_MIN_DEPTH
	
	if len(args) > 1:
		max_depth = int(args[1])
	else:
		max_depth = DEFAULT_MAX_DEPTH

	if not os.path.exists('tiles'):
		os.mkdir('tiles')

	for level in range(min_depth, max_depth):
		print('downloading level %d tiles (%d) ...' % (level, 2**level * 2**level))
		for x in range(0, 2**level):
			if not os.path.exists('tiles/%d/%d' % (level,x)):
				os.makedirs('tiles/%d/%d' % (level,x))
				
			for y in range(0, 2**level):
				url = construct_tile_url(x, y, level)
				download_tile(url, 'tiles/%d/%d/%d.png' % (level, x, y))

def construct_tile_url(x, y, z):
	return 'http://tile.openstreetmap.org/%d/%d/%d.png' % (z, x, y)

def download_tile(url, fname):
	if os.path.exists(fname):
		return
	else:
		ftile = urllib2.urlopen(url)
		fout = open(fname, 'wb')
		fout.write(ftile.read())

if __name__ == '__main__':
	main(sys.argv)
