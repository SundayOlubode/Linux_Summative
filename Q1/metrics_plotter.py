import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from metricsreader import read_metrics
from datetime import datetime

def plot_metrics(filename):
    timestamps, cpu_usage, memory_usage, network_received, network_transmitted = read_metrics(filename)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
    
    # Plot CPU and Memory usage
    ax1.plot(timestamps, cpu_usage, label='CPU Usage (%)', marker='o')
    ax1.plot(timestamps, memory_usage, label='Memory Usage (%)', marker='s')
    ax1.set_title('System Resource Usage Over Time')
    ax1.set_xlabel('Time')
    ax1.set_ylabel('Usage (%)')
    ax1.grid(True)
    ax1.legend()
    
    # Plot Network usage
    ax2.plot(timestamps, network_received, label='Network Received (KB)', marker='^')
    ax2.plot(timestamps, network_transmitted, label='Network Transmitted (KB)', marker='v')
    ax2.set_title('Network Usage Over Time')
    ax2.set_xlabel('Time')
    ax2.set_ylabel('Network Usage (KB)')
    ax2.grid(True)
    ax2.legend()
    
    # readability
    plt.setp(ax1.xaxis.get_majorticklabels(), rotation=45)
    plt.setp(ax2.xaxis.get_majorticklabels(), rotation=45)
    
    plt.tight_layout()
    
    current_time = datetime.now().strftime('%Y%m%d_%H%M%S')
    output_filename = f'system_metrics_{current_time}.png'
    
    plt.savefig(output_filename, dpi=300, bbox_inches='tight')
    print(f"Plot saved as: {output_filename}")

if __name__ == "__main__":
    plot_metrics("usage.txt")