locations = [
	(1,2),
	(2,3),
	(3,4),
	(4,5),
	(5,6),
	(6,7),
	(7,8),
	(8,9)
]

def location_distance(a, b):
	return (b[0] - a[0])**2 + (b[1] - a[1])**2

def find_centroids(locations, radius):
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

def main():
	centroids = find_centroids(locations, 3)
	print_table(centroids)
	print('done!')

def print_table(tbl):
	for k,v in tbl.items():
		print(k,v)
				
if __name__ == '__main__':
	main()
