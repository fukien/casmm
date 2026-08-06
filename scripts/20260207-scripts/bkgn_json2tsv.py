import os

DATA_DIR = "../../dataset/groupby-raw/"

INPUT_FILE = os.path.join(DATA_DIR, "bkgn/book_dataset/raw/ratings.json") 
OUTPUT_FILE = os.path.join(DATA_DIR, "bkgn-ratings.tsv")

user_set = set()
item_set = set()

with open(INPUT_FILE, "r") as fr, open(OUTPUT_FILE, "w") as fw:
	while True:
		line = fr.readline()
		if not line:
			break
		line = line.strip().split(",")
		item_id = line[0].split(" ")[-1]
		user_id = line[1].split(" ")[-1]
		rating = line[2].split(" ")[-1][:-1]
		fw.write("\t".join([user_id, item_id, rating]))
		fw.write("\n")
		user_set.add(user_id)
		item_set.add(item_id)

