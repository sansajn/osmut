'''zobrazi pozicie ATM s mapy (na ploche)
spusti prikazom

	$ python3 vis_atms.py 

'''
import sys, math, osmium
from PySide import QtGui, QtCore
from shapely import geometry

ATM_FILE = 'atm.osm'

class app_window(QtGui.QMainWindow):
	def __init__(self, atms):
		QtGui.QMainWindow.__init__(self, None)
		self._atms = atms
		self._atms_bb = bounding_box(self._atms)
		self._centroids = cluster_centroids(self._atms, 0.02)
		self._zoom = 0.8
		
		self._init_ui()
		
		loc0 = self._atms[0]
		print('loc0', loc0)
		print('bbox', self._atms_bb)
		
	def _init_ui(self):
		self.resize(800, 600)
		self.setWindowTitle("Prague's ATM")

	def paintEvent(self, e):
		qp = QtGui.QPainter()
		qp.begin(self)
		zoom = 0.8
		w,h = self._size()
		offset = (0.1*w, 0.1*h)
		self._draw_atms(zoom, offset, qp)
		self._draw_cluster_centroids(zoom, offset, qp)
		qp.end()

	def _draw_atms(self, zoom, offset, qp):
		qp.save()
		for loc in self._atms:
			x, y = self._to_window_xy(loc, zoom, offset)
			self._draw_atm_mark(x, y, qp)
		qp.restore()

	def _draw_cluster_centroids(self, zoom, offset, qp):
		qp.save()
		for centroid in self._centroids:
			loc = centroid[0]
			x, y = self._to_window_xy(loc, zoom, offset)
			self._draw_cluster_centroid_mark(x, y, 20, qp)
		qp.restore()

	def _size(self):
		s = self.size()
		return s.width(), s.height()

	def _to_window_xy(self, loc, zoom, offset):
		w, h = self._size()
		y_rel = (loc[0] - self._atms_bb[0]) / (self._atms_bb[2] - self._atms_bb[0])
		x_rel = (loc[1] - self._atms_bb[1]) / (self._atms_bb[3] - self._atms_bb[1])
		x = x_rel * w * h / w * zoom + offset[0]
		y = y_rel * h * zoom + offset[1]
		return x, y

	def _draw_atm_mark(self, x, y, qp):
		qp.drawEllipse(QtCore.QPoint(x, y), 3, 3)

	def _draw_cluster_centroid_mark(self, x, y, radius, qp):
		center = QtCore.QPoint(x, y)

		qp.setBrush(QtGui.QBrush())
		qp.drawEllipse(center, radius, radius)

		qp.setBrush(QtGui.QColor(255, 0, 0, 127))
		qp.drawEllipse(center, 3, 3)


class location_handler(osmium.SimpleHandler):
	def __init__(self):
		osmium.SimpleHandler.__init__(self)
		self.locations = []

	def node(self, n):
		loc = n.location
		self.locations.append((loc.lat, loc.lon))

def atm_location(atm_file):
	h = location_handler()
	h.apply_file(atm_file)
	return h.locations

def bounding_box(points):
	p = geometry.MultiPoint(points)
	return p.bounds

def cluster_centroids(atms, radius):
	atms_ltbl = neighbour_count(atms, radius)
	sorted_atms_list = sorted_list(atms_ltbl)
	return find_cluster_centroid(sorted_atms_list, radius)

def neighbour_count(atms, radius):
	'''pocty susedov v okoli, kde radius je angle distance'''
	tbl = {}
	for pos in atms:
		tbl[pos] = 0
	
	max_dist = radius**2
	for a in atms:
		for b in atms:
			if location_distance(a, b) <= max_dist:
				tbl[a] += 1
		tbl[a] -= 1
				
	return tbl  # {(pos), count}

def location_distance(a, b):  # euklid squared distance
	return (b[0] - a[0])**2 + (b[1] - a[1])**2
	
def print_lookup_table(tbl):
	for k,v in tbl.items():
		print(k, v)

def print_list(lst):
	for item in lst:
		print(item)

def sorted_list(tbl):
	lst = [(k,v) for k,v in tbl.items()]
	return sorted(lst, key=lambda row : row[1], reverse=True)

def find_cluster_centroid(lst, radius):
	'''vyfiltrujem vsetky, ktore su od posledneho vzdialene menej ako 2* radius a tie'''
	max_dist = radius
	prev = lst[0]
	result = [lst[0]]
	for i in range(1, len(lst)):
		curr = lst[i]
		if location_distance(prev[0], curr[0]) > max_dist:
			prev = curr
			result.append(curr)
	return result

def main():
	atms = atm_location(ATM_FILE)
	radius = 0.002
	atms_ltbl = neighbour_count(atms, radius)
	sorted_atms_list = sorted_list(atms_ltbl)
	centroids = find_cluster_centroid(sorted_atms_list, radius)
	print_list(sorted_atms_list)
	#print_list(centroids)
	
	app = QtGui.QApplication(sys.argv)
	w = app_window(atms)
	w.show()
	app.exec_()
	

if __name__ == '__main__':
	main()
