''' pokus o vizualizzciu hustoty bodou
spusti prikazom

	$ python3 vis_density.py

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
		self._atms_dense_tbl = sorted_list(neighbour_count(self._atms, 0.002))

		self._init_ui()

	def _init_ui(self):
		self.resize(800, 600)
		self.setWindowTitle("Prague's ATM (dense map)")

	def paintEvent(self, e):
		qp = QtGui.QPainter()
		qp.begin(self)
		zoom = 0.8
		w,h = self._size()
		offset = (0.1*w, 0.1*h)
		self._draw_atms(zoom, offset, qp)
		qp.end()

	def _draw_atms(self, zoom, offset, qp):
		qp.save()
		max_count = self._atms_dense_tbl[0][1]
		for loc, count in self._atms_dense_tbl:
			x, y = self._to_window_xy(loc, zoom, offset)
			self._draw_atm_mark(x, y, 255.0 * (0.2 + 0.8*count/max_count), qp)
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

	def _draw_atm_mark(self, x, y, intensity, qp):
		color = QtGui.QColor(intensity, 0, 0)
		qp.setPen(color)
		qp.setBrush(color)
		qp.drawEllipse(QtCore.QPoint(x, y), 3, 3)

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
	
def sorted_list(tbl):
	lst = [(k,v) for k,v in tbl.items()]
	return sorted(lst, key=lambda row : row[1], reverse=True)

def main():
	atms = atm_location(ATM_FILE)

	app = QtGui.QApplication(sys.argv)
	w = app_window(atms)
	w.show()
	app.exec_()
	

if __name__ == '__main__':
	main()
