import os

input_tsv = "../../dataset/groupby-sorted/clickbench-sorted.csv"
output_dir = "../../dataset/groupby-sorted"
output_csv = os.path.join(output_dir, "chbc_s_skew-sorted.csv")

TARGET_USER_COUNT = 850_000


print("Pass 1: counting rows per user...")
user_row_count = {}
with open(input_tsv, "r") as f:
	header = f.readline().strip().split(",")
	total_users = int(header[0])
	total_urls = int(header[1])
	total_nnz = int(header[2])
	print(f"  Full dataset: {total_users} users, {total_urls} urls, {total_nnz} nnz")

	for line in f:
		parts = line.strip().split(",")
		if len(parts) != 3:
			continue
		user_id = int(parts[0])
		user_row_count[user_id] = user_row_count.get(user_id, 0) + 1

print(f"  Total unique users: {len(user_row_count)}")


sorted_users = sorted(user_row_count.items(), key=lambda x: -x[1])
sampled_user_ids = set(uid for uid, _ in sorted_users[:TARGET_USER_COUNT])
print(f"  Selected top {len(sampled_user_ids)} users by URL visit count")
if len(sorted_users) >= TARGET_USER_COUNT:
	print(f"  Min row count in selected users: {sorted_users[TARGET_USER_COUNT - 1][1]}")


print("Pass 2: filtering rows for selected users...")
rows = []

with open(input_tsv, "r") as f:
	f.readline()
	for line in f:
		parts = line.strip().split(",")
		if len(parts) != 3:
			continue
		user_id = int(parts[0])
		if user_id not in sampled_user_ids:
			continue
		item_id = int(parts[1])
		rating = parts[2]
		rows.append((user_id, item_id, rating))

print(f"  Rows after user sampling: {len(rows)}")
print(f"  Unique URLs after user sampling: {len(set(i for _, i, _ in rows))}")


print("Re-indexing...")


old_user_ids = sorted(set(u for u, _, _ in rows))
old_item_ids = sorted(set(i for _, i, _ in rows))

user_remap = {old: new for new, old in enumerate(old_user_ids)}
item_remap = {old: new for new, old in enumerate(old_item_ids)}

num_users = len(user_remap)
num_urls = len(item_remap)


rows = [(user_remap[u], item_remap[i], r) for u, i, r in rows]
rows.sort(key=lambda x: (x[0], x[1]))

num_nnz = len(rows)
print(f"  Final: {num_users} users, {num_urls} urls, {num_nnz} nnz")


print(f"Writing {output_csv}...")
with open(output_csv, "w") as f:
	f.write(f"{num_users},{num_urls},{num_nnz}\n")
	for u, i, r in rows:

		f.write(f"{u+1},{i+1},{r}\n")

print("Done.")
