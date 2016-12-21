'''vyzualizuje reprezentantou klastrou, kruznicova metoda'''

import sys, math, osmium
from PySide import QtGui, QtCore
from shapely import geometry

ATM_FILE = 'atm.osm'

class app_window(QtGui.QMainWindow):
	def __init__(self, atms):
		QtGui.QMainWindow.__init__(self, None)
		self._atms = atms
		self._atms_bb = bounding_box(self._atms)
		self._radius = 0.01
		self._zoom = 0.8
		self._window_size = (800, 600)
		self._radius_distance_in_px = (self._radius / (self._atms_bb[3] - self._atms_bb[1])) * self._window_size[0]
		self._centroids = find_centroids(self._atms, self._radius*self._radius)  # iba ked je radius < 1

		self._init_ui()
		
		print('distance radius', self._radius_distance_in_px)
		print('atms-bounding-box', self._atms_bb)
		print('atms-bounding-box-width-height', self._atms_bb[3] - self._atms_bb[1], self._atms_bb[2] - self._atms_bb[0])

	def _init_ui(self):
		self.resize(self._window_size[0], self._window_size[1])
		self.setWindowTitle("Prague's ATM (centroids)")

	def paintEvent(self, e):
		qp = QtGui.QPainter()
		qp.begin(self)
		w,h = self._size()
		offset = (0.1*w, 0.1*h)
		self._draw_atms(self._zoom, offset, qp)
		self._draw_centroids(self._zoom, offset, qp)
		qp.end()

	def _draw_centroids(self, zoom, offset, qp):
		qp.save()
		for u, adjs in self._centroids.items():
			loc = self._atms[u]
			x, y = self._to_window_xy(loc, zoom, offset)
			self._draw_centroid_mark(x, y, qp)
		qp.restore()

	def _draw_atms(self, zoom, offset, qp):
		qp.save()
		for loc in self._atms:
			x, y = self._to_window_xy(loc, zoom, offset)
			self._draw_atm_mark(x, y, qp)
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
	
	def _draw_centroid_mark(self, x, y, qp):
		qp.save()
		center = QtCore.QPoint(x, y)
		
		qp.setPen(QtGui.QColor(0, 0, 0))
		qp.drawEllipse(center, self._radius_distance_in_px, self._radius_distance_in_px)
		
		color = QtGui.QColor(255, 0, 0)
		qp.setPen(color)
		qp.setBrush(color)
		qp.drawEllipse(center, 3, 3)
		
		qp.restore()
	

def find_centroids(locations, radius):
	'''radius is squared angle distance'''
	# adjacency structure
	tbl = {}  # {idx:[adj1, adj2, ...]}
	for i in range(0, len(locations)):
		tbl[i] = []
	
	for i in range(0, len(locations)):
		loca = locations[i]
		for j in range(0, len(locations)):
			if i == j:
				continue
			locb = locations[j]
			if location_distance(loca, locb) < radius:
				tbl[i].append(j)
	
	# remove adjacent
	keys = [k for k in tbl.keys()]
	for u in keys:
		if u not in tbl:
			continue
		u_adjs = tbl[u]
		for v in u_adjs:
			if v in tbl:
				v_adjs = tbl[v]
				if len(u_adjs) >= len(v_adjs):
					del tbl[v]
	
	return tbl

def atm_location(atm_file):
	h = location_handler()
	h.apply_file(atm_file)
	return h.locations

def bounding_box(points):
	p = geometry.MultiPoint(points)
	return p.bounds

def location_distance(a, b):
	return (b[0] - a[0])**2 + (b[1] - a[1])**2

class location_handler(osmium.SimpleHandler):
	def __init__(self):
		osmium.SimpleHandler.__init__(self)
		self.locations = []

	def node(self, n):
		loc = n.location
		self.locations.append((loc.lat, loc.lon))

def print_table(tbl):
	for k,v in tbl.items():
		print(k,v)

def main():
	atms = atm_location(ATM_FILE)
	app = QtGui.QApplication(sys.argv)
	w = app_window(atms)
	w.show()
	app.exec_()
	print('done!')


if __name__ == '__main__':
	main()