import os

input_dir = "../../dataset/groupby-raw"
output_dir = "../../dataset/groupby-sorted"

dataset_list = [
	"clickbench",
]


def sort_dataset(dataset):
	input_csv = os.path.join(input_dir, "clickbench_user_url_counts_converted.tsv")
	output_csv = os.path.join(output_dir, "{}-sorted.csv".format(dataset))
	user_iter = 0
	item_iter = 0
	triple_num = 0;
	user2id = {}
	item2id = {}
	user_item_rating_dict = {}

	with open(input_csv, "r") as f:
		header = f.readline()
		while True:
			line = f.readline()
			if not line:
				break
			line = line.strip().split("\t")

			if len(line) != 3:
				continue

			if line[0] not in user2id:
				user2id[line[0]] = user_iter
				user_iter += 1
			if line[1] not in item2id:
				item2id[line[1]] = item_iter
				item_iter += 1

			if user2id[line[0]] not in user_item_rating_dict:
				user_item_rating_dict[user2id[line[0]]] = {}

			if item2id[line[1]] not in user_item_rating_dict[ user2id[line[0]] ]:
				user_item_rating_dict[ user2id[line[0]] ] [item2id[line[1]]] = line[2]
				triple_num += 1


	with open(output_csv, "w") as f:
		f.write(
			",".join(
				[
					str(user_iter),
					str(item_iter),
					str(triple_num)
				]
			)
		)
		f.write("\n")
		for userid in range(len(user2id.keys())):
			for itemid in sorted(user_item_rating_dict[userid].keys()):

				f.write(
					",".join(
						[
							str(userid+1),
							str(itemid+1),
							user_item_rating_dict[userid][itemid]
						]
					)
				)
				f.write("\n")


if __name__ == "__main__":
	if not os.path.exists(output_dir):
		os.makedirs(output_dir)

	for dataset in dataset_list:
		sort_dataset(dataset)
