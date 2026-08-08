import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    pkg_share = get_package_share_directory('secure_telemetry_gateway')
    param_config = os.path.join(pkg_share, 'config', 'gateway_params.yaml')

    gateway_node = Node(
        package='secure_telemetry_gateway',
        executable='secure_telemetry_gateway_node',
        name='secure_telemetry_gateway_node',
        output='screen',
        parameters=[param_config]
    )

    return LaunchDescription([
        gateway_node
    ])