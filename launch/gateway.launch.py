import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    pkg_share = get_package_share_directory('secure_telemetry_gateway')
    default_config_path = os.path.join(pkg_share, 'config', 'gateway_config.yaml')

    gateway_node = Node(
        package='secure_telemetry_gateway',
        executable='secure_telemetry_gateway_node',
        name='secure_telemetry_gateway',
        output='screen',
        emulate_tty=True,
        parameters=[default_config_path]
    )

    return LaunchDescription([
        gateway_node
    ])