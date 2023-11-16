import csv
import matplotlib.pyplot as plt
import math

def q1_a():
    times = [1005.03,1001.62,1000.62,1000.36,1000.15,1000.17,1000.6,1000.17,1000.45,1000.56]
    nums = [i for i in range(1, 11)]
    plt.scatter(nums, times)
    plt.xlabel('time in second')
    plt.ylabel('clock speed')
    plt.show()

def q1_b():
    times = [1000.02,1000.02,1000.03,1000.01,1000.00,1000.01,1000.01,1000.00,1000.01,1000.01]
    nums = [i for i in range(1, 11)]
    plt.scatter(nums, times)
    plt.xlabel('time in second')
    plt.ylabel('clock speed')
    # plt.axis([0, 12, 1000, 1000.5])
    plt.show()


def computer(path):
    busy_time = 0
    total_time = 0
    with open(path, 'r') as file:
        reader = csv.reader(file)
        rowNum = 0
        # total_time = (reader[0][2])-(reader[-1][-1])
        for line in reader:
            if rowNum == 0:
                total_time -= (float)(line[3])
            if rowNum == 499:
                total_time += (float)(line[4])
            busy_time += (float)(line[2])
            rowNum+=1

    return busy_time/ total_time

def get_command():
    sls = []
    for num in range(1, 13):
        s = f'./server 2222 > server-output{num}.txt &./client -a {num} -s 12 -n 500 2222'
        sls.append(s)

    for l in sls:
        print(l)

def computer_all_utilization(start_index, end_index):
    all_utilization = []
    for i in range(start_index, end_index+1):
        path = f"./src/build/data/server-output{i}.csv"
        # print(path)
        data = computer(path)
        all_utilization.append(data)
        # print(data)
    return all_utilization

def get_response(path):
    maxN = 0.0
    minN = 100.0
    mean = 0
    sum = 0
    var = 0
    std = 0
    response_ls = []
    with open(path, 'r') as file:
        reader = csv.reader(file)
        for line in reader:
            response_time = (float)(line[-1])-(float)(line[1])
            response_ls.append(response_time)
            maxN = max(maxN, response_time)
            minN = min(minN, response_time)
            sum += response_time
        mean = sum / 500

        for t in response_ls:
            var += (t - mean)**2
        print(var)
        std = math.sqrt(var)

    return [maxN, minN, mean, std]

def get_all_response_avg(i, j):
    mean = 0
    all_means = []
    for i in range(i, j+1):
        path = f"./src/build/data/server-output{i}.csv"
        mean = get_response(path)[2]
        all_means.append(mean)
    return all_means
    
def plot_graph(x, y):
    plt.plot(x, y)
    plt.xlabel('utilization')
    plt.ylabel('average response time')
    plt.show()

def main():
    # print(get_response("./src/build/data/server-output10.csv"))
    # y = computer_all_utilization(1, 12)
    # x = [i for i in range(1,13)]
    # x = computer_all_utilization(1, 12)
    # y = get_all_response_avg(1, 12)
    # print("x", x)
    # print("y", y)
    # plot_graph(x, y)
    # q1_a()
    # q1_b()

main()